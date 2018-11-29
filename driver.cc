#include <iostream>
#include <stdio.h>
#include <string>
#include <sstream>
#include "unistd.h"
#include <sys/wait.h>
#include <stdlib.h>
using namespace std;
int PID;
string str1;
stringstream strmS;
int main()
{       pid_t childPID;
        int exitStatus; //Exit status of child, used by wait
        pid_t child;    //Pid of child returned by wait
        childPID = fork();
        if (childPID == 0)
        {//Child process
                //print Child PID
                printf("PID of Child = %ld\n", (long) getpid());
                //print Parent PID
                printf("PID of Parent = %ld\n", (long) getppid());
                //generate args for prog1a
                PID = getppid();
                //use stringstream to cast from int to string
                strmS << PID; //put int into stringstream strmS
                strmS >> str1;//extract the string equivalent
                strmS.clear();//clear strmS
                //Execute prog1a
                execlp("/home/timbsn/proj5/client", 
			"name", 
			"/home/timbsn/proj5/orders.txt", 
			"/home/timbsn/proj5/sampleoutput", 
			"einstein.etsu.edu",	
	//		"151.141.91.172",
			"2158",
			"2156",
			NULL);

                //execution failed
                fprintf(stderr, "Execlp failed.\n");
                exit(0);        }
        else if (childPID == -1)
        {//Parent process, if childPID == -1, then there was an error on creation
                fprintf(stderr, "Failed to fork.\n");
                exit(0);
        }
        else //code to be executed by parent
        {//Wait before exiting
                child = wait(&exitStatus);      }
        return 0;
} //End main

