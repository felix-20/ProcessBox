CC=gcc
CFLAGS=-I.
DEPS = pb_memory.h pb_files.h
OBJ = pb_memory.o pb_files.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: all
all: save restore writer clean

save: $(OBJ) save.o
	$(CC) -o $@ $^ $(CFLAGS)

restore: $(OBJ) restore.o
	$(CC) -o $@ $^ $(CFLAGS)

writer: writer.o
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY : clean
clean:
	rm *.o *.bin *.backup