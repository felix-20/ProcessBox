# ProcessBox

We aim to save the state of a running process including one file that's being accessed by this process. Afterwards, we should be able to restore the state of this process, even after a reboot has beed made. Therefor we experimented and tried different possibilities to get information about running processes in order to freeze them.

## main

Our main program saves and restores the virtual memory area and registers of a programm called `writer`. It also backups the file that is used by `writer` and restores also the file.

### signals

We experimented with signals like `SIGSTOP`.

### cgroup

Another possiblity to freeze a process is to force the kernel to swap it. We do this by assigning the process to a cgroup and limit the memory size of this cgroup, so the kernel saves the process on the harddisk.

cgroup - bash script to put a process by its pid into freezer group and freeze the entire group
cgroup_freezer does all of this in one step, letting the terminal sleep for a few seconds.
You can use the other scripts to do this step by step.

### cwd_cmdline

There is the possibilty to only save the current working directory (cwd) and the command line of a process to restart it again later.
