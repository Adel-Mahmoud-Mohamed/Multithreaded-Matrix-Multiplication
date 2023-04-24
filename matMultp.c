#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

// boolean to check whether multiplication is valid or not colA = rowB
bool invalid = false;

bool fileExist = true;
// 3 variables for the dimensions
int x, y, z;

// strings to hold the names passed in argv[]
char mat1_file_name[20];
char mat2_file_name[20];
char out_array_name[20];

// the matrices with initial sizes
int A[20][20], B[20][20], C[20][20];

void initializing_c()
{
    // function to initialize the result matrix before each mult operation
    for (int i = 0; i < x; i++)
    {
        for (int j = 0; j < z; j++)
        {
            C[i][j] = 0;
        }
    }
}

void write_to_file(int mult_type)
{
    // function that writes the result matrix c to a text file
    // file name is passed by the user with the arguments
    // if not passed then we'll assume it to be c.txt

    FILE *out_file;
    // mult_type is a variable to indicate which type of mult we're writing
    // 1 if mult per matrix, 2 if thread per row, 3 if thread per element
    if (mult_type == 1)
    {
        char arr[100] = {};
        strcpy(arr, out_array_name);
        strcat(arr, "_per_matrix.txt");
        out_file = fopen(arr, "w");
        fprintf(out_file, "Method: A thread per matrix\n");
    }
    else if (mult_type == 2)
    {
        char arr[100] = {};
        strcpy(arr, out_array_name);
        strcat(arr, "_per_row.txt");
        out_file = fopen(arr, "w");
        fprintf(out_file, "Method: A thread per row\n");
    }
    else
    {
        char arr[100] = {};
        strcpy(arr, out_array_name);
        strcat(arr, "_per_element.txt");
        out_file = fopen(arr, "w");
        fprintf(out_file, "Method: A thread per element\n");
    }

    fprintf(out_file, "row=%d col=%d\n", x, z);

    for (int i = 0; i < x; i++)
    {
        for (int j = 0; j < z; j++)
        {
            fprintf(out_file, "%d ", C[i][j]);
        }
        fprintf(out_file, "\n");
    }
    fprintf(out_file, "\n");

    fclose(out_file);
}

void print_arr(int rows, int cols, int arr[20][20])
{
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            printf("%d ", arr[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}



void single_thread_mult()
{

    for (int i = 0; i < x; i++)
    {
        for (int j = 0; j < z; j++)
        {
            for (int k = 0; k < y; k++)
            {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

}

void *thread_per_row_routine(void *row_number)
{
    // the thread function for a thread per row of C
    int row = *(int *)row_number;
    for (int i = 0; i < z; i++)
    {
        for (int j = 0; j < y; j++)
        {
            C[row][i] += A[row][j] * B[j][i];
        }
    }
    // free the dynamically allocated variable from the heap
    free(row_number);
}

void thread_per_row_mult()
{
    // function that gets  C = A.B by making a thread to form each row of C
    pthread_t threads[x];
    for (int i = 0; i < x; i++)
    {
        int *row_number = malloc(sizeof(int));
        *row_number = i;
        if (pthread_create(threads + i, NULL, &thread_per_row_routine, (void *)row_number))
        {
            printf("ERROR CREATING THE THREAD!\n");
            exit(-1);
        }
    }
    for (int i = 0; i < x; i++)
    {
        if (pthread_join(threads[i], NULL))
        {
            printf("ERROR JOINING THE THREAD!\n");
            exit(-1);
        }
    }
}

typedef struct element_pos
{
    // structure to hold the position of the target element in C
    // to chose the specific row and col of A and B to mutiply out
    int i;
    int j;
} element_pos;

void *thread_per_element_routine(void *pos)
{
    // the thread function for a thread per element of C
    element_pos *position;
    position = (element_pos *)pos;
    int row = position->i;
    int col = position->j;

    for (int i = 0; i < y; i++)
    {
        C[row][col] += A[row][i] * B[i][col];
    }
    // free the struct pointer at the end
    free(pos);
}

void thread_per_element_mult()
{
    // function that gets  C = A.B by making a thread to form each element of C
    pthread_t threads[x][z];

    for (int i = 0; i < x; i++)
    {
        for (int j = 0; j < z; j++)
        {
            element_pos *pos = malloc(sizeof(element_pos));
            pos->i = i;
            pos->j = j;
            if (pthread_create(&threads[i][j], NULL, &thread_per_element_routine, (void *)pos))
            {
                printf("ERROR CREATING THE THREAD!\n");
                exit(-1);
            }
        }
    }

    for (int i = 0; i < x; i++)
    {
        for (int j = 0; j < z; j++)
        {
            if (pthread_join(threads[i][j], NULL))
            {
                printf("ERROR JOINING THE THREAD!\n");
                exit(-1);
            }
        }
    }
}

void matMult()
{
    // function to get all kind of multiplication by calling the above functions
    // we initialize c at the begining of each mult type
    // record the time taken and print it out with the number of threads taken to perform the multiplication

    struct timeval stop, start;

    initializing_c();
    gettimeofday(&start, NULL);
    single_thread_mult();
    gettimeofday(&stop, NULL);
    write_to_file(1); // 1 indicates type1 mult to open the file names outfilename_per_matrix

    printf("\033[36m");
    printf("\n\n\t\t\tMethod: A thread per matrix\n\n");
    printf("Number of threads created: 1 thread\n\n");
    printf("Microseconds taken: %lu us\n\n", stop.tv_usec - start.tv_usec);
    printf("****************************************************************\n");
    printf("\033[0m");

    initializing_c();
    gettimeofday(&start, NULL);
    thread_per_row_mult();
    gettimeofday(&stop, NULL);
    write_to_file(2);

    printf("\033[31m");
    printf("\t\t\tMethod: A thread per row\n\n");
    printf("Number of threads created: %d threads\n\n", x);
    printf("Microseconds taken: %lu us\n\n", stop.tv_usec - start.tv_usec);
    printf("****************************************************************\n");
    printf("\033[0m");

    initializing_c();
    gettimeofday(&start, NULL);
    thread_per_element_mult();
    gettimeofday(&stop, NULL);
    write_to_file(3);

    printf("\033[32m");
    printf("\t\t\tMethod: A thread per element\n\n");
    printf("Number of threads created: %d threads\n\n", x * z);
    printf("Microseconds taken: %lu us\n\n", stop.tv_usec - start.tv_usec);
    printf("****************************************************************\n");
    printf("\033[0m");
}

void decode_argv(int argc, char const *argv[argc])
{
    // function to get the names of the input files and the desired output file name
    // and that's by looping through argv[] array
    if (argc == 1)
    {
        // means that no arguments are passed so we'll assume a, b for input and c for out.
        strcpy(mat1_file_name, "a.txt");
        strcpy(mat2_file_name, "b.txt");
        strcpy(out_array_name, "c");
    }
    else
    {
        // means that the user has entered the names as arguments
        if (argc < 4)
        {
            // error
            printf("Too few arguments!\n");
            return;
        }
        // extract the names
        strcpy(mat1_file_name, argv[1]);
        strcat(mat1_file_name, ".txt");
        strcpy(mat2_file_name, argv[2]);
        strcat(mat2_file_name, ".txt");
        strcpy(out_array_name, argv[3]);
    }
}

void readA()
{
    // function to get A matrix and its dimensions (x * y) out of iput file associated with A
    FILE *mat1_file, *mat2_file;
    mat1_file = fopen(mat1_file_name, "r");
    if (mat1_file == NULL)
    {
        printf("ERROR NO SUCH FILE!");
        fileExist = false;
        return;
    }
    char singleLine[100];
    int i = 0, count = 0;
    while (!feof(mat1_file))
    {
        fgets(singleLine, 100, mat1_file);
        char tmp[100];
        strcpy(tmp, singleLine);
        if (i == 0)
        {
            char *rowequal_string = strtok(tmp, " ");
            char *colequal_string = strtok(NULL, " ");
            x = atoi(strchr(rowequal_string, '=') + 1);
            y = atoi(strchr(colequal_string, '=') + 1);
            i++;
        }
        else
        {
            if (strcmp(singleLine, "\n") == 0)
            {
                i++;
                continue;
            }
            char *element = strtok(tmp, " \t");
            A[count][0] = atoi(element);
            for (int j = 1; j < y; j++)
            {
                element = strtok(NULL, " \t");
                A[count][j] = atoi(element);
            }
            count++;
            i++;
        }
    }
    fclose(mat1_file);
}

void readB()
{
    // this is pretty much the same as readA() except the destination is different
    FILE *mat2_file;

    mat2_file = fopen(mat2_file_name, "r");
    if (mat2_file == NULL)
    {
        printf("ERROR NO SUCH FILE!");
        fileExist = false;
        return;
    }
    char singleLine[100];
    int i = 0, count = 0;
    while (!feof(mat2_file))
    {
        fgets(singleLine, 100, mat2_file);
        char tmp[100];
        strcpy(tmp, singleLine);
        if (i == 0)
        {
            char *rowequal_string = strtok(tmp, " ");
            char *colequal_string = strtok(NULL, " ");
            int tmp = atoi(strchr(rowequal_string, '=') + 1);
            if (tmp != y)
                invalid = true;
            z = atoi(strchr(colequal_string, '=') + 1);
            i++;
        }
        else
        {
            if (strcmp(singleLine, "\n") == 0)
            {
                i++;
                continue;
            }
            char *element = strtok(tmp, " \t");
            B[count][0] = atoi(element);
            for (int j = 1; j < z; j++)
            {
                element = strtok(NULL, " \t");
                B[count][j] = atoi(element);
            }
            count++;
            i++;
        }
    }
    fclose(mat2_file);
}

int main(int argc, char const *argv[])
{
    decode_argv(argc, argv);
    readA();
    readB();

    if(!fileExist)return 1;

    if (invalid) // means that colA != rowB so nothing can be done
    {
        printf("The given dimensions are not suitable for matrix multiplication!\n");
        return 1;
    }
    
    matMult();
    return 0;
}
