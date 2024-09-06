CC = gcc  
CFLAGS = -Wall -Wextra  
LIBS = -lrt -pthread  


all: md5 view slave

md5: md5.c shmLib.c
	$(CC) -o md5 md5.c shmLib.c $(LIBS) $(CFLAGS)

view: view.c shmLib.c
	$(CC) -o view view.c shmLib.c $(LIBS) $(CFLAGS)

slave: slave.c
	$(CC) -o slave slave.c $(CFLAGS)


clean:
	rm -f md5 view slave
