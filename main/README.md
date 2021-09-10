# Process Box - Main

## Files

This directory includes the following files:

`writer` - is a program that counts to 1000 and writes into a file called `out.txt`.  
`save` - is a program that saves the state of a running process.  
`restore` - is a program that restores the state of a running process.

For both `save` and `restore` you need to provide a process id (pid). If you want to see more options, just don't provide anything and you get an option list.

## How to Use

1. Compile everything by calling `make all`
2. Deactivate ASLR. For instance, you can use the command `echo 0 | sudo tee /proc/sys/kernel/randomize_va_space`
3. Start writer by calling `./writer` and copy the pid of writer that appeared on terminal
4. Save the state of writer by calling `./save <pid> -f -t`. You will notice some files are created automatically
5. Start writer again and copy its pid. You can even do this after a reboot
6. Restore the state of the old writer by calling `./restore <pid> -f`.
