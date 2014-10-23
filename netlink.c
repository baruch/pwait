/*
 * Adapted from http://bewareofgeek.livejournal.com/2945.html
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sysexits.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/connector.h>
#include <linux/cn_proc.h>
#include "pwait.h"

static int create_and_bind_netlink_socket() {
    int rc;
    int nl_sock;
    struct sockaddr_nl sa_nl;

    // create and bind the socket
    nl_sock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
    if (nl_sock == -1) {
        syslog(LOG_CRIT, "Unable to create netlink socket");
        return -1;
    }

    sa_nl.nl_family = AF_NETLINK;
    sa_nl.nl_groups = CN_IDX_PROC;
    sa_nl.nl_pid = getpid();

    rc = bind(nl_sock, (struct sockaddr*)&sa_nl, sizeof(sa_nl));
    if (rc == -1) {
        syslog(LOG_CRIT, "Unable to bind netlink socket");
        close(nl_sock);
        return -1;
    }

    return nl_sock;
}

static int set_socket_status(int nl_sock, int enable) {
    int rc;
    struct __attribute__ ((aligned(NLMSG_ALIGNTO))) {
        struct nlmsghdr header;
        struct __attribute__ ((__packed__)) {
            struct cn_msg message;
            enum proc_cn_mcast_op desired_status;
        };
    } netlink_message;

    memset(&netlink_message, 0, sizeof(netlink_message));
    netlink_message.header.nlmsg_len = sizeof(netlink_message);
    netlink_message.header.nlmsg_pid = getpid();
    netlink_message.header.nlmsg_type = NLMSG_DONE; // indicates the last message in a series

    netlink_message.message.id.idx = CN_IDX_PROC;
    netlink_message.message.id.val = CN_VAL_PROC;
    netlink_message.message.len = sizeof(enum proc_cn_mcast_op);

    netlink_message.desired_status = enable ? PROC_CN_MCAST_LISTEN : PROC_CN_MCAST_IGNORE;

    rc = send(nl_sock, &netlink_message, sizeof(netlink_message), 0);
    if (rc == -1) {
        syslog(LOG_CRIT, "Unable to send message to netlink socket");
        return -1;
    }

    return 0;
}

static volatile int terminate = FALSE;

static int handle_events(int nl_sock, pid_t pid) {
    int rc;
    struct __attribute__ ((aligned(NLMSG_ALIGNTO))) {
        struct nlmsghdr nl_hdr;
        struct __attribute__ ((__packed__)) {
            struct cn_msg cn_msg;
            struct proc_event proc_ev;
        };
    } netlink_message;

    while (!terminate) {
        rc = recv(nl_sock, &netlink_message, sizeof(netlink_message), 0);
        if (rc == 0) {
            // probably means the socket was shut down from the other end
            syslog(LOG_CRIT, "Socket appears to have been shut down");
            return -1;
        }
        else if (rc == -1) {
            if (errno == EINTR) {
                continue;
            }
            syslog(LOG_CRIT, "Error receiving from netlink socket");
            return -1;
        }
        if (netlink_message.proc_ev.what == PROC_EVENT_EXIT && netlink_message.proc_ev.event_data.exit.process_pid == pid) {
            return netlink_message.proc_ev.event_data.exit.exit_code;
        }
    }
    return -1;
}

static void stop_loop(const int signal) {
    terminate = TRUE;
}

int wait_using_netlink(pid_t pid) {
    cap_value_t capability_to_acquire[1];
    struct sigaction siga, oldsiga_term, oldsiga_int;

    if (geteuid() != 0) {
#if defined(CAP_NET_ADMIN)
        capability_to_acquire[0] = CAP_NET_ADMIN;
#else
        syslog(LOG_CRIT, "CAP_NET_ADMIN not available");
        return EX_SOFTWARE;
#endif
        if (!acquire_capabilities(1, capability_to_acquire)) {
            return EX_SOFTWARE;
        }
    }

    int socket = create_and_bind_netlink_socket();
    int handle_return;

    if (set_socket_status(socket, TRUE) == -1) {
        close(socket);
        return EX_OSERR;
    }

    siga.sa_handler = stop_loop;
    sigaction(SIGTERM, &siga, &oldsiga_term);
    sigaction(SIGINT, &siga, &oldsiga_int);

    handle_return = handle_events(socket, pid);

    // Reset the signal handler (hopefully TERM or INT doesn't come right here)
    sigaction(SIGTERM, &oldsiga_term, NULL);
    sigaction(SIGINT, &oldsiga_int, NULL);

    if (handle_return == -1) {
        close(socket);
        return EX_OSERR;
    }
    syslog(LOG_INFO, "Process %d exited with status %d", pid, WEXITSTATUS(handle_return));

    return WEXITSTATUS(handle_return);
}