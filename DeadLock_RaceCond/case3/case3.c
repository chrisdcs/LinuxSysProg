#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<errno.h>
#include<string.h>
#include <linux/ioctl.h>
#include <pthread.h>

#define CDRV_IOC_MAGIC 'Z'
#define E2_IOCMODE1 _IOWR(CDRV_IOC_MAGIC, 1, int)
#define E2_IOCMODE2 _IOWR(CDRV_IOC_MAGIC, 2, int)

#define DEVICE "/dev/a5"

char buf[30];
pthread_mutex_t lock;

pthread_t threads[2];
struct data
{
	int threadId;
	int  filedesc;
};

struct data data1,data2;

void *write_func(void *arg)
{
	struct data *i = (struct data*)arg;
	int id  = i->threadId;
	int fd0 = i->filedesc;
	char buffer[30];
	
	printf("\n Thread id %d has started  writing device \n", id);
        printf("Write trying to acquire lock \n");
    
		pthread_mutex_lock(&lock);
			printf("Write inside crtical region \n");
			strcpy(buffer,buf);
			write(fd0,buffer,sizeof(buf));
		pthread_mutex_unlock(&lock);
		
	printf("Write released lock \n");
	
	printf("Data Written using thread %d to device \n",id);
	pthread_exit(NULL);
 
}

void *read_func(void *arg)
{
	struct data *i = (struct data*)arg;
	int id  = i->threadId;
	int fd0 = i->filedesc;
        char buffer[30];
	
    printf("\n Thread id %d has started reading device \n", id);	
    read(fd0,buffer,sizeof(buffer));	
    printf("Read trying to acquire lock \n");
	
	    pthread_mutex_lock(&lock);
	    strcpy(buf,buffer);
            printf("Read inside critical region \n");
	    pthread_mutex_unlock(&lock);
		
    printf("Read released lock \n");	
    printf("Data Read using thread %d from device \n",id);
	pthread_exit(NULL);
 
}

int main() 
{
	int fd0;
	char* msg;
	
	if (pthread_mutex_init(&lock, NULL) != 0)
        {
        printf("\n mutex init failed\n");
        return 1;
        }
	
	fd0 = open(DEVICE, O_RDWR);
        if(fd0 == -1)
	{
		msg =   strerror(errno);
                printf("File %s either does not exist or has been locked by another process\n", DEVICE);
                fprintf(stderr, "open failed: %s\n", msg);
		exit(-1);
	}
        printf(" Device opened from main thread file descriptor =  %d \n",fd0);
	
	data1.threadId = 0;
	data1.filedesc = fd0;
	pthread_create(&(threads[0]), NULL, write_func, (void*)&data1);
	
	data2.threadId = 1;
	data2.filedesc = fd0;
	pthread_create(&(threads[1]), NULL, read_func, (void*)&data2);
	
	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);
	close(fd0);
        printf(" Device closed from main thread \n");
	
	pthread_mutex_destroy(&lock);
	pthread_exit(NULL);
}
