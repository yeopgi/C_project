//git pull test

// include 
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <poll.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/signal.h>

#include "queue.h"
// include 

// define
#define BUFFSIZE 10
#define QUEUE_SIZE 5
#define DEVICE_PORT "/dev/ttyUSB1"
// define


// function
void* ins_to_queue(void* arg);
void* console_input_poll(void* arg);
void* rem_from_queue();

int print_queue(const Queue *queue);
int init_terminal_attr(struct termios*, 
                       struct termios*,
                       struct termios*, 
                       struct termios* ,
                       int);
// function
                
// variable
Queue queue;
struct pollfd *poll_fd;
pthread_t thread[2];   //input thread, output thread
pthread_mutex_t mutex; // non-busy wait --> both are blocking mutex 
char running_flag;     // thread's running flag
// variable

int main(void)
{    
    void** data = NULL; // pointer to delete queue data

    int thr_id;
    int status;
    
    char test;
    char exit_flag;
    
    struct termios console_fd_terminal_old_attr, console_fd_terminal_new_attr,
                   device_fd_terminal_old_attr, device_fd_terminal_new_attr;
    poll_fd = (struct pollfd* )malloc(sizeof(struct pollfd));

    if (!poll_fd) {
        perror("poll_fd");

        return -1;
    }

    queue_init(&queue, free);

    memset(poll_fd, 0, sizeof(poll_fd));

    poll_fd->fd = open(DEVICE_PORT, O_RDWR | O_NONBLOCK);
       
    printf("poll_fd->fd : %d\n", poll_fd->fd);

    if (poll_fd->fd < 0) {           
        perror(DEVICE_PORT);

        return -1; 
    }           
    
    poll_fd->events = POLLIN;   
    status = init_terminal_attr(&console_fd_terminal_old_attr,   // set poll_fd's attr
                                &console_fd_terminal_new_attr,
                                &device_fd_terminal_old_attr,
                                &device_fd_terminal_new_attr,
                                poll_fd->fd); 
    
    if(status != 0) {
        perror("init terminal");

        close(poll_fd->fd);
        
        free(poll_fd);

        return -1;
    }

    running_flag = 'r'; // 'r' -> run
    thr_id = pthread_create(&thread[0], NULL, ins_to_queue, (void *)&poll_fd->fd); //input thread
    puts("thread[0] starts"); 

    if (thr_id < 0) {
        perror("thr_id");
        
        close(poll_fd->fd);

        free(poll_fd);
     
        return -1;
    }
    
    thr_id = pthread_create(&thread[1], NULL, rem_from_queue, NULL); //input thread
    puts("thread[1] starts"); 

    if (thr_id < 0) {
        perror("thr_id");        

        pthread_mutex_lock(&mutex);
        
        running_flag = 'q';

        pthread_mutex_unlock(&mutex);
        
        pthread_join(thread[0], (void**)&status);

       // set old attr as a terminal's now attr 
        tcsetattr(poll_fd->fd,TCSANOW,&device_fd_terminal_old_attr);
        tcsetattr(STDIN_FILENO,TCSANOW,&console_fd_terminal_old_attr);

        close(poll_fd->fd);

        free(poll_fd);

        return -1;
    }

    puts("to exit program, press ctrl-A");

    while (true) {
        read(STDIN_FILENO, &exit_flag, 1);
        
        if (exit_flag == '\x01') { // insert ctrl-a --> ctrl-c is forbidden due to changing stdin attr
            puts("exit_flag catch");
            pthread_mutex_lock(&mutex);
            puts("running_flag catch");
            running_flag = 'q'; // q --> quit
            pthread_mutex_unlock(&mutex);

            break;
        }
    }

    pthread_join(thread[0], (void**)&status);
    pthread_join(thread[1], (void**)&status);

    // set old attr as a terminal's now attr 
	tcsetattr(poll_fd->fd,TCSANOW,&device_fd_terminal_old_attr);
	tcsetattr(STDIN_FILENO,TCSANOW,&console_fd_terminal_old_attr);

    list_destroy(&queue);

    close(poll_fd->fd);

    free(poll_fd);

    puts("program exit...");
    
    return 0;
}

void* ins_to_queue(void* arg)
{
    char temp_rbuf[BUFFSIZE + 1]; // 임시 버퍼
    char frame_rbuf[BUFFSIZE + 1]; // 프레임 버퍼
    int total_rdcnt = 0;
    int temp_rdcnt = 0;
    int func_ret = -1;    
       
    memset(frame_rbuf, 0, sizeof(frame_rbuf));    

    while (true) {

        memset(temp_rbuf, 0, sizeof(temp_rbuf));
        
        pthread_mutex_lock(&mutex);

        puts("thread[0] catch");
        if (running_flag == 'r') {
            if(!poll(poll_fd, 1, 1000)) {   // poll start
                puts("timeout !!!");

                if(frame_rbuf[0] != '\0') {

                    if(!queue_enqueue(&queue, frame_rbuf)) {

                        printf("in timeout, frame_buf's contents : %s\n", frame_rbuf);

                        puts("inserting to queue is success !!!");

                        memset(frame_rbuf, 0, sizeof(frame_rbuf));

                        total_rdcnt = 0;
                    }
                }

                pthread_mutex_unlock(&mutex);

                usleep(100); //0.0001[s]

                continue;
            }

            puts("polling...");

            switch (poll_fd->revents)  {
                case POLLIN:
                    puts("there is data to read");

                    temp_rdcnt = read(poll_fd->fd, temp_rbuf, BUFFSIZE);

                    printf("temp_rdcnt : %d\n", temp_rdcnt);

                    memcpy(frame_rbuf + total_rdcnt, temp_rbuf, temp_rdcnt);

                    total_rdcnt += temp_rdcnt;

                    if(total_rdcnt == BUFFSIZE) {
                        total_rdcnt = 0;
                        if(!queue_enqueue(&queue, frame_rbuf)) {
                            
                            printf("frame_buf's contents : %s\n", frame_rbuf);

                            puts("inserting to queue is success !!!");
                        }

                        memset(frame_rbuf, 0, sizeof(frame_rbuf));
                    }
                    pthread_mutex_unlock(&mutex);
                    
                    break;
                
                case POLLERR:
                    puts("error occured");

                    break;

                case POLLHUP:
                    puts("disconnected with slave_device");

                    break;

                case POLLNVAL:
                    puts("Invalid request or Invalid fd");

                    break;      

                pthread_mutex_unlock(&mutex);
            }
        } else {
            pthread_mutex_unlock(&mutex);

            usleep(100); //0.0001[s]

            break;
        }

    }
    puts("thread[0] exit...");
}

void* rem_from_queue()
{
    void** data = NULL;

    while(1) {
        pthread_mutex_lock(&mutex);

        puts("thread[1] catch");

        if(running_flag == 'r') { // 'r' -> running
            if(print_queue(&queue) == -1) {
                continue;
            } else if (queue_dequeue(&queue, data) == 0) {
                printf("delete...\n");

                pthread_mutex_unlock(&mutex);                
            }
            
            usleep(100); // 0.0001[s]
        } else {                                           // escape while
            pthread_mutex_unlock(&mutex);

            break;
        }
    }

    puts("thread[1] exit...");
}

int print_queue(const Queue *queue)
{
    ListElmt *element;
    char *data, size;
    fprintf(stdout, "Queue size is %d\n", size = queue_size(queue));

    if(size == 0) {
        pthread_mutex_unlock(&mutex);
        sleep(1);                       // size = 0, wait to insert for 1[s]
        return -1;
    }

    element = list_head(queue);

    data = list_data(element);

    fprintf(stdout, "queue's contents = %s\n", data);

    return size;
}

int init_terminal_attr(struct termios* console_old_terminal_attr, 
                       struct termios* console_new_terminal_attr,
                       struct termios* device_old_terminal_attr, 
                       struct termios* device_new_terminal_attr,
                       int src_fd)
{
    tcgetattr(STDIN_FILENO, console_old_terminal_attr); // get console_fd's current attr to set as a old attr
    
    console_new_terminal_attr->c_cflag = B9600 | CRTSCTS | CS8 | CLOCAL | CREAD;
    console_new_terminal_attr->c_iflag = IGNPAR;
	console_new_terminal_attr->c_oflag = 5; //5
	console_new_terminal_attr->c_lflag = ECHO; //echo
	console_new_terminal_attr->c_cc[VMIN] = 1;  //1s
	console_new_terminal_attr->c_cc[VTIME] = 0;   //0
	
    tcflush(STDIN_FILENO, TCIFLUSH);
	
    tcsetattr(STDIN_FILENO,TCSANOW,console_new_terminal_attr); // set console_fd's current attr
    
    puts("console_fd setting completes");

    tcgetattr(poll_fd->fd, device_old_terminal_attr); // get device_fd's current attr to set as a old attr

    device_new_terminal_attr->c_cflag = B115200 | CS8 | CLOCAL | CREAD;
    device_new_terminal_attr->c_iflag = IGNPAR;
	device_new_terminal_attr->c_oflag = 0;
	device_new_terminal_attr->c_lflag = ECHO;
	device_new_terminal_attr->c_cc[VMIN]=1;
	device_new_terminal_attr->c_cc[VTIME]=0;
	tcflush(poll_fd->fd, TCIFLUSH);

	tcsetattr(poll_fd->fd,TCSANOW,device_new_terminal_attr); // set device_fd's current attr

    puts("device_fd setting completes");
    
    return 0;
}



