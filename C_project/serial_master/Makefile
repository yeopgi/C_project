CC=gcc

serial_master : serial_master.o queue.o list.o
		$(CC) -o serial_master serial_master.o queue.o list.o -lpthread
		@echo "serial_master build complete !!"

serial_master.o : serial_master.c
		$(CC) -c serial_master.c -lpthread
		@echo "serial_master.c --> serial_master.o compile complete !!!"	

queue.o : queue.c
		$(CC) -c queue.c 
		@echo "queue.c --> queue.o compile complete !!!"	

list.o : list.c
		$(CC) -c list.c 
		@echo "list.c --> list.o compile complete !!!"	

clean:
	rm -rf serial_master *.o
	@echo "all delete..."
	
