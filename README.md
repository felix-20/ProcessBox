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