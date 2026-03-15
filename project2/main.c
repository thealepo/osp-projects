#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// defining the arrays
int *unsorted_array;
int *sorted_array;

// thread struct
typedef struct{
    int start;
    int end;
}thread_data;

// global variables
int array_size;

// ----- FUNCTIONS -----

// sorting function (bubble-sort)
void *bubble_sort(void *arg){
    thread_data *data = (thread_data *)arg;

    int start = data->start;
    int end = data->end;

    for (int i = start ; i < end-1 ; i++){
        for (int j = start ; j < end-1-(i-start) ; j++){
            if (unsorted_array[j] > unsorted_array[j+1]){
                int temp = unsorted_array[j];
                unsorted_array[j] = unsorted_array[j+1];
                unsorted_array[j+1] = temp;
            }
        }
    }
    pthread_exit(0);
}

void merge(void *arg){
    thread_data *data = (thread_data *)arg;

    // initialize indices
    int start1 = data->start;
    int end1 = (data->start + data->end) / 2;
    int start2 = end1;
    int end2 = data->end;

    // two pointer
    int i = start1;
    int j = start2;
    int k = start1;

    // merge
    while (i < end1 && j < end2){
        if (unsorted_array[i] <= unsorted_array[j]){
            sorted_array[k++] = unsorted_array[i++];
        }else{
            sorted_array[k++] = unsorted_array[j++];
        }
    }

    // copy remainders
    while (i < end1){
        sorted_array[k++] = unsorted_array[i++];
    }
    while (j < end2){
        sorted_array[k++] = unsorted_array[j++];
    }

    pthread_exit(0);
}

// main function
int main(void){

    // input array size
    printf("Enter the size of the array: ");
    scanf("%d" , &array_size);

    unsorted_array = malloc(array_size * sizeof(int));
    sorted_array = malloc(array_size * sizeof(int));

    // input array elements
    printf("Enter the elements of the array: ");
    for (int i = 0 ; i < array_size ; i++){
        scanf("%d" , &unsorted_array[i]);
    }

    printf("Unsorted array: ");
    for (int i = 0 ; i < array_size ; i++){
        printf("%d " , unsorted_array[i]);
    }
    printf("\n");

    // threads
    pthread_t tid1 , tid2 , tid3;
    thread_data data1 = {0 , array_size/2};
    thread_data data2 = {array_size/2 , array_size};

    pthread_create(&tid1 , NULL , bubble_sort , &data1);
    pthread_create(&tid2 , NULL , bubble_sort , &data2);

    // wait
    pthread_join(tid1 , NULL);
    pthread_join(tid2 , NULL);

    thread_data data3 = {0 , array_size};
    pthread_create(&tid3 , NULL , merge , &data3);

    pthread_join(tid3 , NULL);

    printf("Sorted array: ");
    for (int i = 0 ; i < array_size ; i++){
        printf("%d " , sorted_array[i]);
    }
    printf("\n");
    

    free(unsorted_array);
    free(sorted_array);
    return 0;
}