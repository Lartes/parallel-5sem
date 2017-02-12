#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <vector>
#include <string>

//this structure is got by each thread
struct ThreadInfo
{
    long long int elementsCount;
    long long int startIndex;
    int sizeRow;
    double *matrix;
    pthread_t id;
};

struct point
{
    point(int x, int y)
    {
        this->x = x;
        this->y = y;
    };
    int x,y;
};

double min, max;
std::vector<point> minPoints, maxPoints;
pthread_mutex_t mutexMin;
pthread_mutex_t mutexMax;
pthread_mutex_t mutexMinPoints;
pthread_mutex_t mutexMaxPoints;

void *thread_function (void *args)
{
    ThreadInfo *thread = (ThreadInfo *) args;
    pthread_t tid = pthread_self ();
    long long int startIndex = thread->startIndex;
    int sizeRow = thread->sizeRow;
    double localMax = thread->matrix[0];
    double localMin = thread->matrix[0];
    std::vector<point> localMinPoints, localMaxPoints;
    localMaxPoints.push_back(point((startIndex)/sizeRow,(startIndex)%sizeRow));
    localMinPoints.push_back(point((startIndex)/sizeRow,(startIndex)%sizeRow));
    for (long long int i = 1; i<thread->elementsCount; i++){
        if (thread->matrix[i]>=localMax)
            if (thread->matrix[i]>localMax)
            {
                localMax = thread->matrix[i];
                localMaxPoints.clear();
                localMaxPoints.push_back(point((i+startIndex)/sizeRow,(i+startIndex)%sizeRow));
            }else
            {
                localMaxPoints.push_back(point((i+startIndex)/sizeRow,(i+startIndex)%sizeRow));
            }
        if (thread->matrix[i]<=localMin)
            if (thread->matrix[i]<localMin)
            {
                localMin = thread->matrix[i];
                localMinPoints.clear();
                localMinPoints.push_back(point((i+startIndex)/sizeRow,(i+startIndex)%sizeRow));
            }else
            {
                localMinPoints.push_back(point((i+startIndex)/sizeRow,(i+startIndex)%sizeRow));
            }
    }
    if (localMax>max)
    {
        pthread_mutex_lock(&mutexMax);
        max = localMax;
        pthread_mutex_unlock(&mutexMax);
        pthread_mutex_lock(&mutexMaxPoints);
        maxPoints = localMaxPoints;
        pthread_mutex_unlock(&mutexMaxPoints);
    }
    if (localMax==max)
    {
        pthread_mutex_lock(&mutexMaxPoints);
        for(int j=0; j<localMaxPoints.size(); j++)
            maxPoints.push_back(localMaxPoints[j]);
        pthread_mutex_unlock(&mutexMaxPoints);
    }
    if (localMin<min)
    {
        pthread_mutex_lock(&mutexMin);
        min = localMin;
        pthread_mutex_unlock(&mutexMin);
        pthread_mutex_lock(&mutexMinPoints);
        minPoints = localMinPoints;
        pthread_mutex_unlock(&mutexMinPoints);
    }
    if (localMin==min)
    {
        pthread_mutex_lock(&mutexMinPoints);
        for(int j=0; j<localMinPoints.size(); j++)
            minPoints.push_back(localMinPoints[j]);
        pthread_mutex_unlock(&mutexMinPoints);
    }
    //printf("id=%lu min=%f max=%f start=%d number=%d\n",tid, localMin, localMax, startIndex, thread->elementsCount);
}

int main (int argc, char **argv)
{
    /*
    Possible variants of input:
    without arguments
    <number of threads>
    <first number of thread> <last number of thread> <step size between first and last threads>
    */

    //INITIALISATION
    long long int threadsCount;
    if(argc==2)
    {
        threadsCount = atoi (argv[1]);
    }
    else
    {
        printf("Number of threads doesn't exist\n");
        exit(0);
    }

    ThreadInfo threads[threadsCount];
    int sizeRow, sizeCol;
    long long int elementsCount, elementsCountLeft;
    double *matrix;
    FILE *input;
    FILE *output;

    //READING
    input = fopen ("in", "r");
    output = fopen ("data.csv", "a");
    if (input == NULL)
    {
        printf("File doesn't exist\n");
        exit(0);
    }
    std::string text;
    text.resize(6);
    fread((void*)text.c_str(), sizeof(char), 6, input);
    if(text!="MATRIX"){
        printf("Your file isn't valid.\n");
        exit(0);
    }
    fread(&sizeRow, sizeof(int), 1, input);
    fread(&sizeCol, sizeof(int), 1, input);
    elementsCount = sizeCol*sizeRow;
    matrix = (double *) calloc (elementsCount, sizeof (*matrix));

    for (int j = 0; j < sizeRow; j++)
        for (int k = 0; k < sizeCol; k++)
            fread(&(matrix[k+sizeCol*j]), sizeof(double), 1, input);
    fclose (input);
    elementsCountLeft = elementsCount;

    max = matrix[0];
    min = matrix[0];
    maxPoints.clear();
    minPoints.clear();
    maxPoints.push_back(point(0,0));
    minPoints.push_back(point(0,0));


    //PROCESSING

    struct timeval timeStart, timeEnd;
    gettimeofday(&timeStart, NULL);

    for (long long int threadIndex = 0; threadIndex < threadsCount - 1; threadIndex++)
    {
        threads[threadIndex].elementsCount = elementsCountLeft / (threadsCount - threadIndex);
        threads[threadIndex].matrix = &(matrix[elementsCount - elementsCountLeft]);
        threads[threadIndex].startIndex = elementsCount - elementsCountLeft;
        threads[threadIndex].sizeRow = sizeRow;
        elementsCountLeft -= elementsCountLeft / (threadsCount - threadIndex);
        pthread_create (&(threads[threadIndex].id), 0, thread_function, &(threads[threadIndex]));
    }
    threads[threadsCount - 1].elementsCount = elementsCountLeft;
    threads[threadsCount - 1].matrix = &(matrix[elementsCount - elementsCountLeft]);
    threads[threadsCount - 1].startIndex = elementsCount - elementsCountLeft;
    threads[threadsCount - 1].sizeRow = sizeRow;
    pthread_create (&(threads[threadsCount - 1].id), 0, thread_function, &(threads[threadsCount - 1]));
    pthread_join (threads[threadsCount - 1].id, 0);

    for (int threadIndex = 0; threadIndex < threadsCount - 1; threadIndex++)
    {
        pthread_join (threads[threadIndex].id, 0);
    }

    gettimeofday(&timeEnd, NULL);
    unsigned long int time = (timeEnd.tv_sec*1e6 + timeEnd.tv_usec)-(timeStart.tv_sec*1e6 + timeStart.tv_usec);

    //PRINTING
    /*
    printf("max = %f\n", max);
    for (int j = 0; j<maxPoints.size(); j++){
        printf("%dx%d ",maxPoints[j].x,maxPoints[j].y);
    }
    printf("\n");
    printf("min = %f\n", min);
    for (int j = 0; j<minPoints.size(); j++){
        printf("%dx%d ",minPoints[j].x,minPoints[j].y);
    }
    printf("\n");
    */
    fprintf(output,"%lu,",time);
    //printf("********************************************************************\n");

    fclose(output);
    free(matrix);
    return 0;
}
