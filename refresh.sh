echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
rm freeze_fd/heap.bin
rm freeze_fd/registers.bin
rm freeze_fd/stack.bin
rm freeze_fd/file.backup
rm freeze_fd/writer
rm freeze_fd/save
rm freeze_fd/restore
gcc freeze_fd/writer.c -o freeze_fd/writer
gcc freeze_fd/save.c -o freeze_fd/save
gcc freeze_fd/restore.c -o freeze_fd/restore