#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <math.h>
#include <sys/time.h>
#include "functions.h"

using std::string;
using std::cout;

/*
double getPrev(double* matrix, int i, int j, int sizeRow, int sizeCol, double t, double dx, double dy){
    if (i==sizeCol || i==0 || j==0 || j==sizeRow){
        return border(j*dx,i*dy,t);
    }
    return matrix(i,j);
}
*/

double heat (double x, double y, double t);

double init (double x, double y);

double border (double x, double y, double t, double X, double Y);

int main()
{

    //INITIALIZATION
    int sizeRow, sizeCol;
    long long int elementsCount;
    double *matrix, *matrixPrev;
    FILE *input;
    FILE *output;
    double a = 0.5;
    double t = 0;
    double heatBorder;

    double x, y;
    double dx = 1;
    double dy = 1;
    double dt = 1;
    double tWrite = 1;
    double tEnd = 1;
    char buffer[30];
    string prefix;


    //READING CONFIGURATION
    input = fopen ("config", "r");
    if (input == NULL)
    {
        printf("File with configarations doesn't exist\n");
        exit(0);
    }
    fscanf(input, "%*[^0-9.]");
    fscanf(input, "%lf %lf",&dx, &x);
    fscanf(input, "%*[^0-9.]");
    fscanf(input, "%lf %lf",&dy, &y);
    fscanf(input, "%*[^0-9.]");
    fscanf(input, "%lf",&dt);
    fscanf(input, "%*[^0-9.]");
    fscanf(input, "%lf",&tWrite);
    fscanf(input, "%*[^0-9.]");
    fscanf(input, "%lf",&tEnd);
    fscanf(input, "%*[^a-zA-Z._]");
    fscanf(input, "%[a-zA-Z._]",buffer);
    prefix = string(buffer);
    fclose (input);

    FILE *timeFile;
    timeFile = fopen ("time.csv", "a");
    struct timeval timeStart, timeEnd;
    gettimeofday(&timeStart, NULL);

    //CREATING INITIAL MATRIX
    sizeRow = round(x / dx + 1); // size of row of full matrix
    sizeCol = round(y / dy + 1); // size of column of full matrix

    elementsCount = sizeRow*sizeCol;
    matrix = (double *) calloc (elementsCount, sizeof (*matrix));
    matrixPrev = (double *) calloc (elementsCount, sizeof (*matrixPrev));
    for (int j=0; j < sizeCol; j++){
        for (int i=0; i < sizeRow; i++){
            matrixPrev[i+sizeRow*j] = init(i*dx, j*dy);
        }
        matrixPrev[0+sizeRow*j] = 0;
        matrixPrev[sizeRow-1+sizeRow*j] = 0;
    }
    for (int i=0; i < sizeRow; i++){
        matrixPrev[i+sizeRow*0] = 0;
        matrixPrev[i+sizeRow*(sizeCol-1)] = 0;
    }

    //PROCESSING
    do
    {
        for(int q=0; q<tWrite/dt; q++)
        {
            //BORDERS
            for (int i = 0; i < sizeCol; i++)
            {
                heatBorder = border(0,i,t,sizeRow, sizeCol);
                if (heatBorder == -1){
                    matrix[0+sizeRow*i] = matrix[1+sizeRow*i];
                }
                else{
                    matrix[0+sizeRow*i] = heatBorder;
                }

                heatBorder = border((sizeRow-1),i,t,sizeRow, sizeCol);
                if (heatBorder == -1){
                    matrix[sizeRow-1+sizeRow*i] = matrix[sizeRow-2+sizeRow*i];
                }
                else{
                    matrix[sizeRow-1+sizeRow*i] = heatBorder;
                }
            }
            for (int j = 1; j < sizeRow-1; j++)
            {
                heatBorder = border(j,0,t,sizeRow, sizeCol);
                if (heatBorder == -1){
                    matrix[j+sizeRow*0] = matrix[j+sizeRow*1];
                }
                else{
                    matrix[j+sizeRow*0] = heatBorder;
                }

                heatBorder = border(j,(sizeCol-1),t,sizeRow, sizeCol);
                if (heatBorder == -1){
                    matrix[j+sizeRow*(sizeCol-1)] = matrix[j+sizeRow*(sizeCol-2)];
                }
                else{
                    matrix[j+sizeRow*(sizeCol-1)] = heatBorder;
                }
            }
            //BODY
            for (int j = 1; j < sizeCol-1; j++)
            {
                for (int i = 1; i < sizeRow-1; i++)
                {
                    double grad = a*a * ((matrixPrev[(i-1)+sizeRow*j]-2*matrixPrev[i+sizeRow*j]+matrixPrev[(i+1)+sizeRow*j])/(dx*dx) +
                                         (matrixPrev[i+sizeRow*(j-1)]-2*matrixPrev[i+sizeRow*j]+matrixPrev[i+sizeRow*(j+1)])/(dy*dy)) +
                                  heat(i*dx,j*dy,t);
                    matrix[i+sizeRow*j] = grad * dt + matrixPrev[i+sizeRow*j];
                }
            }
            t+=dt;
            for (int j = 0; j < sizeCol; j++)
                for (int i = 0; i < sizeRow; i++)
                {
                    matrixPrev[i+sizeRow*j]=matrix[i+sizeRow*j];
                }
        }
//cout << "t=" << t << "\n";
        //WRITING
        sprintf(buffer,"%.1f",t);
        string name = prefix + string(buffer);
        output = fopen (name.c_str(), "w");
        double sizeRowD = double(sizeRow);
        double sizeColD = double(sizeCol);
        fwrite(&sizeRowD, sizeof(double), 1, output);
        fwrite(&sizeColD, sizeof(double), 1, output);
        for (int j = 0; j < sizeCol; j++)
        {
            for (int i = 0; i < sizeRow; i++)
            {
                //cout << (int)matrix[i+sizeRow*j] << " ";
                fwrite(&(matrix[i+sizeRow*j]), sizeof(double), 1, output);
            }
            //cout << "\n";
        }
        fclose (output);

    }while (t<=tEnd);

    gettimeofday(&timeEnd, NULL);
    unsigned long int time = (timeEnd.tv_sec*1e6 + timeEnd.tv_usec)-(timeStart.tv_sec*1e6 + timeStart.tv_usec);
    fprintf(timeFile,"%d,%lu,",sizeRow,time);
    fclose(timeFile);

    free(matrix);
    free(matrixPrev);
    return 0;
}
