/*
 * Project.c is an encryption program that takes an inputted text file
 * and runs multiple threads concurrently to place it in a buffer of
 * user specified size, encrpyts all letters in a random amount, and then
 * places the now encypted letters in a specified output text file, with
 * random resets of the encryption key throughout the process. It will also
 * output counts of the amount of each character at every reset as well as
 * at the end of the program
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include "encrypt.h"

int buffersize;           //size of buffer
int flag;                 //flag for halting threads during reset
char *input;              //input buffer
char *output;             //output buffer
sem_t inputsemfree;       //semaphore for making sure an input can be overwritten
sem_t inputsemplace;      //semaphore for making sure an input has been placed in buffer
sem_t inputsemcount;      //semaphore for allowing inputs to be counted
sem_t inputsemcountdone;  //semaphore for ensuring inputs are not overwritten before counted
sem_t outputsemfree;      //semaphore for making sure an output can be overwritten
sem_t outputsemplace;     //semaphore for making sure an output has been placed in buffer
sem_t outputsemcount;     //semaphore for allowing outputs to be counted
sem_t outputsemcountdone; //semaphore for ensuring outputs are not overwritten before counted

void printinput()
{ //prints the current input counts in format specified by documentation
    printf("Input file contains\n");
    for (int i = 65; i < 91; i++)
    {
        printf("%c:%d ", i, get_input_count(i));
    }
    printf("\n");
}

void printoutput()
{ //prints the current output counts in format specified by documentation
    printf("Output file contains\n");
    for (int i = 65; i < 91; i++)
    {
        printf("%c:%d ", i, get_output_count(i));
    }
    printf("\n");
}

void *reading()
{
    char next;
    int iterator = 0;
    int val;
    next = read_input();
    while (1)
    { //while the End of File is not found
        while (flag)
            ;                                //flag for reset halt
        sem_wait(&inputsemfree);             //wait for free spot in buffer
        sem_wait(&inputsemcountdone);        //wait to not overwrite before counted
        input[iterator % buffersize] = next; //set next character in buffer
        sem_post(&inputsemplace);            //open semaphores
        sem_post(&inputsemcount);
        if (next == EOF)
        { //break at end of file
            break;
        }
        next = read_input();
        iterator++;
    }
}

void *writing()
{
    int iterator = 0;
    while (1)
    { //while the End of File is not found
        while (flag)
            ;                                     //flag for reset halt
        sem_wait(&outputsemplace);                //wait for output buffer to have new chars
        sem_wait(&outputsemcountdone);            //wait for done counting
        if (output[iterator % buffersize] == EOF) //break at end of file
        {
            sem_post(&outputsemfree); //open semaphores
            sem_post(&outputsemcount);
            break;
        }
        write_output(output[iterator % buffersize]); //write to output file
        sem_post(&outputsemfree);                    //open semaphores
        sem_post(&outputsemcount);
        iterator++;
    }
}

void *encrypting()
{
    int iterator = 0;
    while (1)                            //while end of file is not found
    {
        while (flag)//flag for reset halt
            ;
        sem_wait(&inputsemplace);        //wait for input and output
        sem_wait(&outputsemfree);
        output[iterator % buffersize] = caesar_encrypt(input[iterator % buffersize]);  //place encrypted input in output
        sem_post(&inputsemfree);         //open input and output
        sem_post(&outputsemplace);
        if (output[iterator % buffersize] == EOF)
        {
            break;                         //exit at end of file
        }
        iterator++;
    }
}

void *inputcounting()
{
    int iterator = 0;
    while (1)
    {                                 //while end of file is not found
        while (flag) //flag for reset halt
            ;
        sem_wait(&inputsemcount);         //wait for input
        if (input[iterator % buffersize] == EOF) //if end of file is found, release semaphore and exit
        {
            sem_post(&inputsemcountdone);
            break;
        }
        count_input(input[iterator % buffersize]); //count input
        sem_post(&inputsemcountdone);              //open semaphore
        iterator++;
    }
}

void *outputcounting()
{
    int iterator = 0;
    while (1)                    //while end of file is not found
    {
        while (flag) //flag for reset halt
            ;
        sem_wait(&outputsemcount);  //wait for output
        if (output[iterator % buffersize] == EOF) //if end of file is found, release semaphore and exit
        {
            sem_post(&outputsemcountdone);
            break;
        }
        count_output(output[iterator % buffersize]); //count input
        sem_post(&outputsemcountdone);               //open semaphore
        iterator++;
    }
}

void reset_requested()
{
    flag = 1; //halt processes
    printinput();
    printoutput();
}

void reset_finished()
{
    printf("Reset finished\n");
    flag = 0; //resume processes
}

int main(int argc, char const *argv[])
{
    if (argc != 3) //check number of arguments
    {
        printf("Incorrect amount of arguments\nRequires input and output files\nExiting...\n");
        return 0;
    }

    open_input(argv[1]);  //open files
    open_output(argv[2]);

    printf("Please enter buffer size: "); //get buffer size
    scanf("%d", &buffersize);

    if (buffersize < 1) //error check buffer size
    {
        printf("Buffer size must be an int greater than 0\nExiting...\n");
        return 0;
    }

    input = malloc(sizeof(char) * buffersize);  //initialize input buffer
    output = malloc(sizeof(char) * buffersize); //initialize output buffer

    flag = 0;                                   //initialize resest flag

    pthread_t readerthread;                     //create threads for each process
    pthread_t writerthread;
    pthread_t encrypterthread;
    pthread_t inputcounterthread;
    pthread_t outputcounterthread;

    sem_init(&inputsemfree, 0, buffersize);    //initialize all semaphores from global variables
    sem_init(&inputsemplace, 0, 0);
    sem_init(&inputsemcount, 0, 0);
    sem_init(&inputsemcountdone, 0, buffersize);
    sem_init(&outputsemfree, 0, buffersize);
    sem_init(&outputsemplace, 0, 0);
    sem_init(&outputsemcount, 0, 0);
    sem_init(&outputsemcountdone, 0, buffersize);

    pthread_create(&readerthread, NULL, reading, NULL); //start threads
    pthread_create(&writerthread, NULL, writing, NULL);
    pthread_create(&encrypterthread, NULL, encrypting, NULL);
    pthread_create(&inputcounterthread, NULL, inputcounting, NULL);
    pthread_create(&outputcounterthread, NULL, outputcounting, NULL);
    
    pthread_join(readerthread, NULL);          //wait for threads to finish
    pthread_join(writerthread, NULL);
    pthread_join(encrypterthread, NULL);
    pthread_join(inputcounterthread, NULL);
    pthread_join(outputcounterthread, NULL);
    
    printinput();                   //print output according to documentation
    printoutput();
    printf("End of file reached\n");
    return 0;
}
