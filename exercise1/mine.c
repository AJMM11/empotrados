#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>
#include <sys/mman.h>

//A ITERA 40 VECES Y SUMA A LA VARIABLE COMPARTIDA 100
//B ITERA 40 VECES Y SUMA A A LA VARIABLE COMPARTIDA 1
//TIENEN LA MISMA PRIORIDAD
//PROBAR EN FIFO Y ROUND ROBIN
//PROBAR CON PRIORIDADES DIFERENTES EN ROUND ROBIN

//Inicializar las variables del problema
#define PRIO_A 40
#define ITER_A 10
#define INC_A 100
#define PER_A 1

#define PRIO_B 40
#define ITER_B 10
#define INC_B 1
#define PER_B 1

#define PRIO_0 (PRIO_A+PRIO_B)
//Crear struct con las variables que necesitemos modificar y un mutex
struct Data{
    int cnt;
    pthread_mutex_t mutex;
};

void *tareaA(void *arg){
    struct Data* data = (struct Data *)arg;

    int veces = ITER_A;

    struct timespec periodo;
    periodo.tv_sec = PER_A;
    periodo.tv_nsec = 0;

    struct timespec next;

//Coge el tiempo actual y lo guarda en la variable next
    clock_gettime(CLOCK_MONOTONIC,&next);

    while(veces>0){
    //Sección crítica
        pthread_mutex_lock(&data->mutex);
        data->cnt= (data->cnt)+INC_A;
        printf("A: %d \n",data->cnt);
        veces--;
        pthread_mutex_unlock(&data->mutex);

    //Sumar periodo de A a next (para que espere el tiempo que debe)
    next.tv_sec = next.tv_sec+PER_A;
    next.tv_nsec = next.tv_nsec+0;

    //Convertir nsec a sec para que no haya overflow
    next.tv_sec+=next.tv_nsec/1000000000;
    next.tv_nsec=next.tv_nsec%1000000000;
    //Espera hasta que llegue next
    clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&next,NULL);
    }
    return NULL;
}

void *tareaB(void *arg){
//Inicializar datos compartidos
    struct Data * data = (struct Data *) arg;

    int veces = ITER_B;

//Inicializar estructuras tiempo
    struct timespec next;
    struct timespec periodo;

    periodo.tv_sec=PER_B;
    periodo.tv_nsec=0;

//almacenar tiempo actual en next
    clock_gettime(CLOCK_MONOTONIC,&next);
    while(veces>0){
        pthread_mutex_lock(&data->mutex);
        data->cnt+=INC_B;
        printf("B: %d \n",data->cnt);
        veces--;
        pthread_mutex_unlock(&data->mutex);

        next.tv_sec+=PER_B;
        next.tv_nsec+=0;

        next.tv_sec+=next.tv_nsec/1000000000;
        next.tv_nsec= next.tv_nsec%1000000000;

        clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&next,NULL);
    }
    return NULL;
}
int main(){

    pthread_t tA,tB;
    struct sched_param param;
    pthread_attr_t attr;
    int pcy,policy=SCHED_RR;

    struct Data data;
//Bloquear memoria de todos los procesos
(mlockall(MCL_CURRENT | MCL_FUTURE));
//get the current scheduling parameters of the main thread
pthread_getschedparam(pthread_self(),&pcy,&param);
//Set the priority of the main thread to the highest
param.sched_priority=PRIO_0;
pthread_setschedparam(pthread_self(),policy, &param);
//Inicializa los datos compartidos
    data.cnt=0;
    pthread_mutex_init(&data.mutex,NULL);

//Inicializa la política de planificación
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr,SCHED_RR);

//INICIALIZA LAS TAREAS(X FIN!!!)

//Le asigna la prioridad a la tarea A y crea la hebra
    param.sched_priority = PRIO_A;
    pthread_attr_setschedparam(&attr,&param);
    pthread_create(&tA,&attr,tareaA,&data);

//Igual con B
    param.sched_priority = PRIO_B;
    pthread_attr_setschedparam(&attr,&param);
    pthread_create(&tB,&attr,tareaB,&data);

//Inicia las hebras
    pthread_join(tA,NULL);
    pthread_join(tB,NULL);

 
    return 0;

}