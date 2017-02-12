#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string>

FILE* out;

int main (int argc, char **argv)
{
    out = fopen("in", "wb");
    std::string text("MATRIX");
    fwrite(text.c_str(), sizeof(char), 6 ,out);
    if (argc != 3)
        return 1;
    int row = (int) atoi (argv[1]);
    int col = (int) atoi (argv[2]);
    fwrite(&row, sizeof(int),1,out);
    fwrite(&col, sizeof(int),1,out);
    for (int i = 0; i < row; i++)
    {
        for (int j = 0; j < col; j++)
        {
            double tmp = (double)rand();
            fwrite(&tmp, sizeof(double),1,out);
        }
    }
    fclose(out);
    return 0;
}
