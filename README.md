# ProcessBox

## our goal
We want to take a running process and save it into a file.
Afterwards we want to take the file and restore the process.
Therefor we researched different possibilities to get information about running processes and freezing them.

## tests programs
test - a little counter, can be used to test different approaches

### using kill
send signals - able to send signals to processes, using signals and kill

### cgroup
freezer cgroup - bash script to put a process by its pid into freezer group and freeze the entire group
cgroup_freezer does all of this in one step, letting the terminal sleep for a few seconds.
You can use the other scripts to do this step by step

### ptrace & stack (reloaded)
save.c is a script to save not only the cmline and cwd but also the whole stack and the stack pointer. Use -s to stop the process afterwards. With restore.c you can use these information to restore the process. Don't forget to deactivate ASLR in Linux by executing ```echo 0 | sudo tee /proc/sys/kernel/randomize_va_space```