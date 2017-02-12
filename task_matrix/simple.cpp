#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <vector>
#include <string>

struct point
{
    point(int x, int y)
    {
        this->x = x;
        this->y = y;
    };
    int x,y;
};

int main ()
{
    int sizeRow, sizeCol;
    long long int elementsCount;
    double *matrix;
    std::vector<point> minPoints, maxPoints;
    FILE *input;
    FILE *output;
    FILE *timeFile;

    //READING
    input = fopen ("in", "r");
    output = fopen ("data.csv", "a");
    timeFile = fopen ("time.csv", "a");
    if (input == NULL)
    {
        printf("File doesn't exist\n");
        exit(0);
    }
    std::string text;
    text.resize(6);
    fread((void*)text.c_str(), sizeof(char), 6, input);
    if(text!="MATRIX")
    {
        printf("Your file isn't valid.\n");
        exit(0);
    }
    fread(&sizeRow, sizeof(int), 1, input);
    fread(&sizeCol, sizeof(int), 1, input);
    elementsCount = sizeCol*sizeRow;
    matrix = (double *) calloc (elementsCount, sizeof (*matrix));

    struct timeval timeStart, timeEnd;
    gettimeofday(&timeStart, NULL);

    for (int j = 0; j < sizeRow; j++)
        for (int k = 0; k < sizeCol; k++)
            fread(&(matrix[k+sizeCol*j]), sizeof(double), 1, input);

    gettimeofday(&timeEnd, NULL);
    unsigned long int time = (timeEnd.tv_sec*1e6 + timeEnd.tv_usec)-(timeStart.tv_sec*1e6 + timeStart.tv_usec);
    fprintf(timeFile,"%lu,",time);

    fclose (timeFile);
    fclose (input);

    double max = matrix[0];
    double min = matrix[0];
    maxPoints.push_back(point(0,0));
    minPoints.push_back(point(0,0));

    //PROCESSING
    gettimeofday(&timeStart, NULL);

    for (long long int i = 1; i<elementsCount; i++){
        if (matrix[i]>=max)
        {
            if (matrix[i]>max)
            {
                max = matrix[i];
                maxPoints.clear();
                maxPoints.push_back(point(i/sizeRow,i%sizeRow));
            }else
            {
                maxPoints.push_back(point(i/sizeRow,i%sizeRow));
            }
        }
        if (matrix[i]<=min)
        {
            if (matrix[i]<min)
            {
                min = matrix[i];
                minPoints.clear();
                minPoints.push_back(point(i/sizeRow,i%sizeRow));
            }else
            {
                minPoints.push_back(point(i/sizeRow,i%sizeRow));
            }
        }
    }
    //printf("id=%lu min=%f max=%f start=%d number=%d\n",tid, localMin, localMax, startIndex, thread->elementsCount);

    gettimeofday(&timeEnd, NULL);
    time = (timeEnd.tv_sec*1e6 + timeEnd.tv_usec)-(timeStart.tv_sec*1e6 + timeStart.tv_usec);

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
    fprintf(output,"%d,%lu,",sizeRow,time);
    //printf("********************************************************************\n");

    fclose(output);
    free(matrix);
    return 0;
}
