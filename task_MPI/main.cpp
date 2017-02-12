#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <mpi.h>
#include <stddef.h>
#include <math.h>
#include <sys/time.h>
#include "functions.h"

using std::string;
using std::cout;

double ost(double x, double y){
    return abs(x-floor(x/y+0.0001)*y);
}

double heat (double x, double y, double t);

double init (double x, double y);

double border (double x, double y, double t, double X, double Y);

typedef struct {
    double dx;
    double dy;
    double dt;
    double x;
    double y;
    int tasksX;
    int tasksY;
    double tWrite;
    double tEnd;
    char buffer[30];
} Initial;

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    Initial in;

    int tasksAll, myTask;
    MPI_Comm_size(MPI_COMM_WORLD, &tasksAll);
    MPI_Comm_rank(MPI_COMM_WORLD, &myTask);

    if (myTask==0){
        if(argc!=3){
            printf("There are no input params\n");
            MPI_Abort(MPI_COMM_WORLD, MPI_ERR_OTHER);
            return 0;
        }
        else{
            in.tasksX = atoi(argv[1]);
            in.tasksY = atoi(argv[2]);
            if(in.tasksX*in.tasksY!=tasksAll){
                printf("Input params aren't correct: %d * %d != %d\n",in.tasksX, in.tasksY, tasksAll);
                MPI_Abort(MPI_COMM_WORLD, MPI_ERR_OTHER);
                return 0;
            }
            else{
                //printf("Input params are correct\n");
            }
        }
    }

    //INITIALIZATION
    int sizeRow, sizeCol, sizeRowAll, sizeColAll;
    long long int elementsCount;
    double *matrix, *matrixPrev, *toChange;
    FILE *input;
    double a = 0.5;
    double t = 0;
    double heatBorder;

    char buffer[30];
    string prefix;

    MPI_Status status;

    MPI_Datatype InitialType;
    int len[11] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 30, 1};
    MPI_Aint pos[11] = {offsetof(Initial,dx), offsetof(Initial,dy),
                        offsetof(Initial,dt), offsetof(Initial,x),
                        offsetof(Initial,y), offsetof(Initial,tasksX),
                        offsetof(Initial,tasksY), offsetof(Initial,tWrite),
                        offsetof(Initial,tEnd),offsetof(Initial,buffer),
                        sizeof(Initial)};
    MPI_Datatype typ[11] = {MPI_DOUBLE,MPI_DOUBLE,MPI_DOUBLE,MPI_DOUBLE,MPI_DOUBLE,MPI_INT,MPI_INT,MPI_DOUBLE, MPI_DOUBLE, MPI_CHAR, MPI_UB};
    MPI_Type_struct(11, len, pos, typ, &InitialType);
    MPI_Type_commit(&InitialType);

    if (myTask == 0){
        //READING CONFIGURATION
        input = fopen ("config", "r");
        if (input == NULL)
        {
            printf("File with configarations doesn't exist\n");
            MPI_Abort(MPI_COMM_WORLD, MPI_ERR_OTHER);
            return 0;
        }
        fscanf(input, "%*[^0-9.]");
        fscanf(input, "%lf %lf",&in.dx, &in.x);
        fscanf(input, "%*[^0-9.]");
        fscanf(input, "%lf %lf",&in.dy, &in.y);
        fscanf(input, "%*[^0-9.]");
        fscanf(input, "%lf",&in.dt);
        fscanf(input, "%*[^0-9.]");
        fscanf(input, "%lf",&in.tWrite);
        fscanf(input, "%*[^0-9.]");
        fscanf(input, "%lf",&in.tEnd);
        fscanf(input, "%*[^a-zA-Z._]");
        fscanf(input, "%[a-zA-Z._]",in.buffer);
        fclose (input);
    }

    MPI_Bcast(&in, 1, InitialType, 0, MPI_COMM_WORLD);
    prefix = string(in.buffer);

    FILE *timeFile;
    struct timeval timeStart, timeEnd;
    if(myTask==0){
        timeFile = fopen ("time.csv", "a");
        gettimeofday(&timeStart, NULL);
    }

    //CREATING INITIAL MATRIX
    if (myTask == 0){
        if (ost(in.x, in.dx) != 0 || ost(in.y, in.dy) != 0){
            printf("Incorrect x - dx or y - dy\n");
            MPI_Abort(MPI_COMM_WORLD, MPI_ERR_OTHER);
            return 0;
        }
    }

    sizeRowAll = round(in.x / in.dx + 1); // size of row of full matrix
    sizeColAll = round(in.y / in.dy + 1); // size of column of full matrix

    int taskX = myTask % in.tasksX; // number of block on X axis for this task
    int taskY = myTask / in.tasksX; // number of block on Y axis for this task

    int startX = 0; // number of start position in array for this task
    int lastX = sizeRowAll;
    int stopX = lastX / in.tasksX - 1; // number of elements to process in this task
    for(int i=1; i<=taskX; i++){
        startX += lastX / (in.tasksX - i +1);
        lastX -= lastX / (in.tasksX - i + 1);
        stopX += lastX / (in.tasksX - i);
    }
    int startY = 0;  // number of start position in array for this task
    int lastY = sizeColAll;
    int stopY = lastY / in.tasksY - 1; // number of elements to process in this task
    for(int i=1; i<=taskY; i++){
        startY += lastY / (in.tasksY - i +1);
        lastY -= lastY / (in.tasksY - i + 1);
        stopY += lastY / (in.tasksY - i);
    }

    sizeRow = stopX-startX+3; // size of row of this part of matrix (including borders)
    sizeCol = stopY-startY+3; // size of column of this part of matrix (including borders)

//printf("myTask=%d taskX=%d taskY=%d startX=%d stopX=%d startY=%d stopY=%d\n",myTask, taskX, taskY, startX, stopX, startY, stopY);
//fflush(stdout);

    elementsCount = sizeRow*sizeCol;
    matrix = (double *) calloc (elementsCount, sizeof (*matrix));
    matrixPrev = (double *) calloc (elementsCount, sizeof (*matrixPrev));
    for (int j=0; j+startY <= stopY; j++){
        for (int i=0; i+startX <= stopX; i++){
            matrixPrev[i+1+sizeRow*(j+1)] = init((i+startX)*in.dx, (j+startY)*in.dy);
        }
        matrixPrev[0+sizeRow*(j+1)] = 0;
        matrixPrev[sizeRow-1+sizeRow*(j+1)] = 0;
    }
    for (int i=0; i+startX <= stopX; i++){
        matrixPrev[i+1+sizeRow*0] = 0;
        matrixPrev[i+1+sizeRow*(sizeCol-1)] = 0;
    }
    toChange = (double *) calloc (sizeCol, sizeof (*toChange));

//MPI_Barrier(MPI_COMM_WORLD);

    //PROCESSING
    int left = myTask - 1;
    int right = myTask + 1;
    int top = myTask - in.tasksX;
    int bottom = myTask + in.tasksX;

    do
    {
        for(int q=0; q<in.tWrite/in.dt; q++)
        {
            t+=in.dt;
//printf("myTask=%d START MESSAGING, t=%lf\n", myTask, t);
//fflush(stdout);
//MPI_Barrier(MPI_COMM_WORLD);
            //LEFT BORDER
            if (myTask % in.tasksX == 0)
            {
                for (int j = 0; j + startY <= stopY; j++)
                {
                    heatBorder = border(0,(j + startY)*in.dy,t, in.x, in.y);
                    if (heatBorder == -1)
                    {
                        matrix[0+sizeRow*(j+1)] = matrix[1+sizeRow*(j+1)];
                    }
                    else
                    {
                        matrix[0+sizeRow*(j+1)] = heatBorder;
                    }
                }
            }else
            {
                for(int j = 0; j + startY <= stopY; j++)
                {
                    toChange[j] = matrix[1+sizeRow*(j+1)];
                }
                MPI_Send(toChange, sizeCol-2, MPI_DOUBLE, left, 0, MPI_COMM_WORLD);
//printf("myTask=%d SEND LEFT %d\n", myTask, left);
//fflush(stdout);
            }
            //RIGHT BORDER
            if ((myTask+1) % in.tasksX == 0)
            {
                for (int j = 0; j + startY <= stopY; j++)
                {
                    heatBorder = border(in.x,(j + startY)*in.dy,t,in.x, in.y);
                    if (heatBorder == -1)
                    {
                        matrix[sizeRow-1+sizeRow*(j+1)] = matrix[sizeRow-2+sizeRow*(j+1)];
                    }
                    else
                    {
                        matrix[sizeRow-1+sizeRow*(j+1)] = heatBorder;
                    }
                }
            }else
            {
                MPI_Recv(toChange, sizeCol-2, MPI_DOUBLE, right, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
//printf("myTask=%d RECV RIGHT %d\n", myTask, right);
//fflush(stdout);
                for(int j = 0; j + startY <= stopY; j++)
                {
                    matrix[sizeRow-1+sizeRow*(j+1)] = toChange[j];
                    toChange[j] = matrix[sizeRow-2+sizeRow*(j+1)];
                }
                MPI_Send(toChange, sizeCol-2, MPI_DOUBLE, right, 0, MPI_COMM_WORLD);
//printf("myTask=%d SEND RIGHT %d\n", myTask, right);
//fflush(stdout);
            }
            //LEFT BORDER
            if (myTask % in.tasksX != 0)
            {
                MPI_Recv(toChange, sizeCol-2, MPI_DOUBLE, left, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
//printf("myTask=%d RECV LEFT %d\n", myTask, left);
//fflush(stdout);
                for(int j = 0; j + startY <= stopY; j++)
                {
                    matrix[0+sizeRow*(j+1)] = toChange[j];
                }
            }
            //TOP BORDER
            if(myTask < in.tasksX)
            {
                for (int i = 0; i + startX <= stopX; i++)
                {
                    heatBorder = border((i + startX)*in.dx,0,t,in.x, in.y);
                    if (heatBorder == -1)
                    {
                        matrix[i+1+sizeRow*0] = matrix[i+1+sizeRow*1];
                    }
                    else
                    {
                        matrix[i+1+sizeRow*0] = heatBorder;
                    }
                }
            }else
            {
                MPI_Send(&matrix[1+sizeRow*1], sizeRow-2, MPI_DOUBLE, top, 0, MPI_COMM_WORLD);
//printf("myTask=%d SEND TOP %d\n", myTask, top);
//fflush(stdout);
            }
            //BOTTOM BORDER
            if(tasksAll-myTask <= in.tasksX)
            {
                for (int i = 0; i + startX <= stopX; i++)
                {
                    heatBorder = border((i + startX)*in.dx,in.y,t,in.x, in.y);
                    if (heatBorder == -1)
                    {
                        matrix[i+1+sizeRow*(sizeCol-1)] = matrix[i+1+sizeRow*(sizeCol-2)];
                    }
                    else
                    {
                        matrix[i+1+sizeRow*(sizeCol-1)] = heatBorder;
                    }
                }
            }else
            {
                MPI_Recv(&matrix[1+sizeRow*(sizeCol-1)], sizeRow-2, MPI_DOUBLE, bottom, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
//printf("myTask=%d RECV BOTTOM %d\n", myTask, bottom);
//fflush(stdout);
                MPI_Send(&matrix[1+sizeRow*(sizeCol-2)], sizeRow-2, MPI_DOUBLE, bottom, 0, MPI_COMM_WORLD);
//printf("myTask=%d SEND BOTTOM %d\n", myTask, bottom);
//fflush(stdout);
            }
            //TOP BORDER
            if(myTask >= in.tasksX)
            {
                MPI_Recv(&matrix[1+sizeRow*0], sizeRow-2, MPI_DOUBLE, top, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
//printf("myTask=%d RECV TOP %d\n", myTask, top);
//fflush(stdout);
            }
//MPI_Barrier(MPI_COMM_WORLD);
//printf("myTask=%d COMPLETE MESSAGING, t=%lf\n", myTask, t);
//fflush(stdout);

            //BODY
            for (int j = 0; j + startY <= stopY; j++)
            {
                for (int i = 0; i + startX <= stopX; i++)
                {
                    double grad = a*a * ((matrixPrev[(i)+sizeRow*(j+1)]-2*matrixPrev[i+1+sizeRow*(j+1)]+matrixPrev[(i+2)+sizeRow*(j+1)])/(in.dx*in.dx) +
                                         (matrixPrev[i+1+sizeRow*(j)]-2*matrixPrev[i+1+sizeRow*(j+1)]+matrixPrev[i+1+sizeRow*(j+2)])/(in.dy*in.dy)) +
                                  heat((i+startX)*in.dx,(j+startY)*in.dy,t);
                    matrix[i+1+sizeRow*(j+1)] = grad * in.dt + matrixPrev[i+1+sizeRow*(j+1)];
                }
            }
            for (int j = 0; j < sizeCol; j++)
                for (int i = 0; i < sizeRow; i++)
                {
                    matrixPrev[i+sizeRow*j]=matrix[i+sizeRow*j];
                }
//printf("myTask=%d COMPLETE SOLVING, t=%lf\n", myTask,t);
//fflush(stdout);
        }
//printf("myTask=%d COMPLETE PART, t=%lf\n", myTask, t);
//fflush(stdout);
        //WRITING
        sprintf(buffer,"%.1f",t);
        string name = prefix + string(buffer);
        MPI_File fileout;
//MPI_Barrier(MPI_COMM_WORLD);
//printf("myTask=%d PREPARE TO WRITING\n", myTask);
//fflush(stdout);
        MPI_File_open(MPI_COMM_SELF, name.c_str(), MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &fileout);

//printf("myTask=%d START WRITING\n", myTask);
//fflush(stdout);
        if(myTask==0){
            double towrite[] = {double(sizeRowAll),double(sizeColAll)};
            MPI_File_set_view(fileout, 0*sizeof(double), MPI_DOUBLE, MPI_DOUBLE, "native", MPI_INFO_NULL);
            MPI_File_write(fileout, towrite, 2, MPI_DOUBLE, &status);
        }
        for(int j = 0; j + startY <= stopY; j++)
        {
            //printf("myTask=%d write from %d to %d: %lf %lf\n", myTask, startX + (startY+j)*(sizeRowAll), startX + (startY+j)*(sizeRowAll)+sizeRow-2-1, towrite[1+sizeRow*(j+1)], towrite[1+sizeRow*(j+1)+1]);
            //fflush(stdout);
            MPI_File_set_view(fileout, (startX + (startY+j)*(sizeRowAll) + 2)*sizeof(double), MPI_DOUBLE, MPI_DOUBLE, "native", MPI_INFO_NULL);
            MPI_File_write(fileout, &matrix[1+sizeRow*(j+1)], sizeRow-2, MPI_DOUBLE, &status);
        }


//printf("myTask=%d END WRITING\n", myTask);
//fflush(stdout);
//MPI_Barrier(MPI_COMM_WORLD);
        MPI_File_close(&fileout);
//printf("myTask=%d FILE CLOSED\n", myTask);
//fflush(stdout);

    }while (t<=in.tEnd);

    unsigned long int  time;
    if(myTask==0){
        gettimeofday(&timeEnd, NULL);
        time = (timeEnd.tv_sec*1e6 + timeEnd.tv_usec)-(timeStart.tv_sec*1e6 + timeStart.tv_usec);
        fprintf(timeFile,"%lu,",time);
        fclose(timeFile);
    }

    free(matrix);
    free(matrixPrev);
    free(toChange);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Type_free(&InitialType);
    MPI_Finalize();

    return 0;
}
