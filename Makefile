CC = gcc  
CFLAGS = -Wall -Wextra  
LIBS = -lrt -pthread  


all: md5 view slave

md5: md5.c shm_lib.c
	$(CC) -o md5 md5.c shm_lib.c $(LIBS) $(CFLAGS)

view: view.c shm_lib.c
	$(CC) -o view view.c shm_lib.c $(LIBS) $(CFLAGS)

slave: slave.c
	$(CC) -o slave slave.c $(CFLAGS)


clean:
	rm -f md5 view slave
