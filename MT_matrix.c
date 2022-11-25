#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>


int **m1=NULL;
int **m2=NULL;
long long int **m3 = NULL;
int n = 0;
int m = 0;
int k = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *thread_operation(void *vargp);

/*use struct I define to transfer variable */
typedef struct my_data{
    int thread_rank;
    int block_size;
    int col;
}my_data;

void *thread_operation(void *vargp){
    my_data *data = (my_data*)vargp;
    int rank = data->thread_rank;// 0~127
    int block_size = data->block_size;// 4096/32=128
    int col = data->col;
    // printf("m: %d\n",m);
    for(int i = 0;i<block_size && block_size * rank+i<m ;i++){
        for(int j=0;j<col;j++){
            for(int k=0;k<n;k++){
                m3[block_size*rank+i][j] += m1[block_size*rank+i][k] * m2[k][j]; 
            }
        }
    }
    pthread_mutex_lock(&mutex);//lock
    FILE *fp = fopen("//proc//thread_info","w");
    pid_t tid = syscall(SYS_gettid);
    // printf("%d\n",tid);
    fprintf(fp,"%d\n",tid);
    fclose(fp);
    pthread_mutex_unlock(&mutex);//unlock
    

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {      //input format: ./MY_proc.c thread_number m1 m2
    char *c1 = argv[2];
    char *c2 = argv[3];
    n = 0;m = 0;k = 0;
    size_t t;
    char *c = NULL;
    char *p = NULL;
    char *d = " \n";
    int thread_number = atoi(argv[1]);
    int every_thread_capacity = 0;
    struct timespec t1,t2;
    long int interval;
    pthread_t pthread;

    /* read in m1 */
    FILE *fp1 = fopen(c1,"r");
    getline(&c,&t,fp1);
    p = strtok(c,d);
    m = atoi(p);
    every_thread_capacity = m/thread_number;
    if(m%thread_number!=0)
        every_thread_capacity++;
    p = strtok(NULL,d);
    n = atoi(p);
    m1 = (int**)malloc(m*sizeof(int*));
    for(int i=0;i<m;i++){
        m1[i] = (int*)malloc(sizeof(int) * n);
        for(int j=0;j<n;j++){
            m1[i][j] = 0;
        }
        getline(&c,&t,fp1);
        p = strtok(c,d);
        m1[i][0] = atoi(p);
        for(int j=1;j<n;j++){
            p = strtok(NULL,d);
            m1[i][j] = atoi(p);
        }
    }
    fclose(fp1);
    
    
    /* read in m2 */
    FILE *fp2= fopen(c2,"r");
    getline(&c,&t,fp2);
    p = strtok(c,d);
    n = atoi(p);
    p = strtok(NULL,"\n");
    k = atoi(p);
    m2 = (int**)malloc(sizeof(int*) * n);
    for(int i=0;i<n;i++){
        m2[i] = (int*)malloc(sizeof(int) * k);
        for(int j=0;j<k;j++){
            m2[i][j] = 0;
        }
        
        getline(&c,&t,fp2);
        p = strtok(c,d);
        m2[i][0] = atoi(p);
        for(int j=1;j<k;j++){
            p = strtok(NULL,d);
            m2[i][j] = atoi(p);
        }
    }
    fclose(fp2);

    /* malloc m3 space */
    m3 = (long long int**)malloc(sizeof(long long int*) * m);
    for(int i=0;i<m;i++){
        m3[i] = (long long int*)malloc(sizeof(long long int) * k);
        for(int j=0;j<k;j++){
            m3[i][j] = 0;
        }
    }

    /* create thread */
    
    pthread_t thread_id[thread_number];
    my_data data[thread_number];

    clock_gettime(CLOCK_MONOTONIC, &t1);
    for(int j=0;j<thread_number;j++){
            data[j].thread_rank = j;
            data[j].block_size = every_thread_capacity;
            data[j].col = k;
            // printf("%d %d %d %d\n",j,every_thread_capacity,thread_number,n);
            pthread_create(&thread_id[j], NULL, thread_operation,(void*)&data[j]);
            // FILE *fp = fopen("/proc/thread_info","w+");
            // fprintf(fp,"%d",getpid());
            // fclose(fp);
    }
    for(int i=0;i<thread_number;i++){
        pthread_join(thread_id[i],NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &t2);
    interval = 1000000* (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec)/1000;
    printf("Time: %ldms\n",interval/1000);

    // printf("test\n");
    /* write m3 to result.txt */
    FILE *fp = fopen("result.txt","w");
    fprintf(fp, "%d %d\n", m,k);
    for(int i=0;i<m;i++){
        for(int j=0;j<k;j++){
            fprintf(fp,"%lld ",m3[i][j]);
        }
        fprintf(fp,"\n");
    }
    // printf("test\n");

    /* print pid */
    printf("PID: %d\n",getpid());

    /* write thread_id to thread_info */
    FILE *fp_proc = fopen("/proc/thread_info","r");
    char kernel_to_user[150];
    while(fgets(kernel_to_user,65,fp_proc)!=NULL){
        printf("%s",kernel_to_user);
    }

    return 0;
}