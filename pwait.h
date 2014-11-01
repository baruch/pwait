/***************************************************************************
 *   Copyright (C) 2014 by David Zaslavsky <diazona@ellipsix.net>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#pragma once

#include <signal.h>
#include <sys/capability.h>

#define TRUE 1
#define FALSE 0

/**
 * Make a best effort to ensure that the process has a certain set of
 * capabilities. If it already does, or if this function was able to set all
 * of the requested capabilities, this returns 1 (TRUE). Otherwise, this
 * returns 0 (FALSE).
 *
 * The function is a bit long because it checks for error codes at every step,
 * but conceptually what it does for each requested capability is very
 * straightforward:
 * 1. Check whether the capability is supported by the kernel
 * 2. Check whether the process already has the capability set, and if so,
 *    return TRUE
 * 3. Check whether the process is allowed to give itself the capability, and
 *    if not, return FALSE
 * 4. Attempt to actually set the capability
 * 5. Check again to make sure the process has the capability, and return
 *    FALSE if not
 */
int acquire_capabilities(size_t n, const cap_value_t* capabilities_to_acquire);

int wait_using_ptrace(pid_t pid);
int wait_using_netlink(pid_t pid);
