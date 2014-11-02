pwait_poll() {
    while [ -d /proc/$1 ]; do
        sleep ${2:-5}
    done
}
