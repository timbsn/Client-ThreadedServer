///////////////////////////////////////////////////////////////////////////////
//Nathan Timbs CSCI 4727 prog5server.cc 4/01/10
//This program runs as a daemon server. It takes one command line parameter,
//which is assigned to the port number. It accepts connections using a socket.
//The server attempts to establish another connection to the client.
//The server accepts product records from the client, processes the records,
//and then sends them back to the client using a different connection. 
//The program uses queues to pass the records from station to station and to the
//writer thread then back to the client.
//Upon accepting a connection, it creates a thread, which in turn creates five
//more threads, which are used to process the various calculations 
//described in projects 2 and 3. Upon receipt of a record with an id number 
//of -1, the processes exit, displaying their individual reports. 
///////////////////////////////////////////////////////////////////////////////
#include <sys/wait.h>
#include <stdlib.h>
#include "unistd.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <strings.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <queue>
#include <string>
using namespace std;
#define MAXSTAGES 5		//The number of stations in the program
#define PRODUCTSIZE 128		//Product Name Buffer Size
int prodRec[MAXSTAGES];		//used by the children to keep statistics
struct product_record {
        int idnumber;           // Unique identification
        char name[PRODUCTSIZE]; // String description
        double price;           // Unit cost
        int number;             // Number ordered
        double tax;             // Tax on order
        double sANDh;           // Shipping and handling
        double total;           // Total order cost
        int stations[MAXSTAGES];// Stations processed
        };//end struct
product_record myProduct;
product_record myProductq;
product_record myProductAft;
double runningTotal=0;
// Thread function to handle one request
void* handleRequest( void* arg );
char hostname2[81];		//used in server -> client connection
char portnum2[81];		//used in server -> client connection
void* fnthread0(void* parameter);//thread 0 function
void* fnthread1(void* parameter);//thread 1 function
void* fnthread2(void* parameter);//thread 2 function
void* fnthread3(void* parameter);//thread 3 function
void* fnthread4(void* parameter);//thread 4 function
void* fnthread5(void* parameter);//thread 5 function
void* fnthread6(void* parameter);//thread 6 function
void* fnthread7(void* parameter);//thread 7 function
queue<product_record> myqueue[8];//declare an array of queues
pthread_t p[7];//declare seven thread
pthread_mutex_t mylock[9] = PTHREAD_MUTEX_INITIALIZER; //initialize nine mutexes
sem_t mysem[9]; //declare nine semaphores
int reccount = 0;

struct thread_args {
	int id;
	};//end struct
//    From Stevens, Unix Network Programming
	pid_t master;		//used by master
	int masterExitStatus;	//used by master
	int n = MAXSTAGES;	//used by wait() for children	
        int exitStatus;		//used by wait() for children
        pid_t childPID[MAXSTAGES];	//used by fork()
	pid_t masterPID;	//used by wait()
        pid_t child;		//used by wait()

int main(int argc, char** argv )
{
	int sockdesc;		//used to create socket
	struct addrinfo* myinfo;//used to create connection address
	char portnum[81];	//used to create port number
	int connection;		//used for connection
	int value;		//used in master code	
	pthread_t thrd;

        //initialize semaphores
        for (int k = 0; k < 9; k++)
        {
                sem_init(&mysem[k], 0, 0);
        }
	//assign port number from cmd line for receiving connection
	strcpy(portnum, argv[1]);
   	// Use AF_UNIX for unix pathnames instead
   	// Use SOCK_DGRAM for UDP datagrams
   	sockdesc = socket(AF_INET, SOCK_STREAM, 0);
   	if ( sockdesc < 0 )
   	{
   	   	cout << "S1 Error creating socket" << endl;
   	   	exit(0);
   	}
   	// Set up the address record
   	if ( getaddrinfo( "0.0.0.0", portnum, NULL, &myinfo ) != 0 )
   	{
   	   	cout << "S1 Error getting address" << endl;
   	   	exit(0);
   	}
   	// Bind the socket to an address
   	if (bind(sockdesc, myinfo->ai_addr, myinfo->ai_addrlen) < 0 )
   	{
      		cout << "S1 Error binding to socket" << endl;
      		exit(0);
   	}
   	// Now listen to the socket
   	if ( listen(sockdesc, 1) < 0 )
	{
      		cout << "S1 Error in listen" << endl;
      		exit(0);
	}

   	// Accept a connect, check the port number
	for(;;)
   	{
   		connection = accept(sockdesc, NULL, NULL);
      		if ( connection < 0 )
     		{
         		cout << "Error in accept" << endl;
         		exit(0);
      		}//end if
      		else
      		{//deal with the request
			pthread_create(&thrd, NULL, handleRequest, (void*)&connection);
		}//end else
	}//end for
	return 0;
}//end main

//thread function to create station threads and writer thread
void* handleRequest(void* arg)
{
	int i = 0;
	int connection;
	int value;
	connection = *((int*)arg);
	pthread_detach( pthread_self() );
        struct thread_args fn_args;

        //create a thread to calculate tax and check for special order numbers
        if (pthread_create(&p[1], NULL, fnthread1, &fn_args) < 0)
        {
                cout << "Thread 1 Creation Error" << endl;
                exit (0);
        }//end if
        //create a thread to calculate chipping and handling
        if (pthread_create(&p[2], NULL, fnthread2, &fn_args) < 0)
        {
                cout << "Thread 2 Creation Error" << endl;
                exit (0);
        }//end if
        //create a thread to calculate the total of the order
        if (pthread_create(&p[3], NULL, fnthread3, &fn_args) < 0)
        {
                cout << "Thread 3 Creation Error" << endl;
                exit (0);
        }//end if
        //create a thread to keep a running total of all orders
        if (pthread_create(&p[4], NULL, fnthread4, &fn_args) < 0)
        {
                cout << "Thread 4 Creation Error" << endl;
                exit (0);
        }//end if
        //create a thread to display the order informatip,
        if (pthread_create(&p[5], NULL, fnthread5, &fn_args) < 0)
        {
                cout << "Thread 5 Creation Error" << endl;
                exit (0);
        }//end if

        while(myProduct.idnumber !=-1 )
        {
                value = read(connection, (void*)&myProduct, sizeof(product_record));
		if (reccount < 1)
		{
			//assign the portnum2 and hostname2 values
			sprintf(portnum2,"%d", myProduct.idnumber);
			sscanf(myProduct.name, "%s", hostname2);

        		//create a thread to write the processed records to the output file
        		if (pthread_create(&p[6], NULL, fnthread6, (void*)&connection/* outFile*/) < 0)
        		{
                		cout << "Thread 6 Creation Error" << endl;
                		exit (0);
       			}//end if
		}//end if
                if ( value < 0 )
                {
                        cout << "S2: Error on recv" << endl;
                        exit(0);
                }//end if
                else if ( value == 0 )
                {
                        cout << "End of transmission" << endl;
                        exit(0);
                }//end else if
                else
                {
                        //process the record
                        //write the record to child 1 if this isnt the first record
			if (reccount > 0)
			{
				pthread_mutex_lock(&mylock[1]);
				//write the record to the queue
				myqueue[1].push(myProduct);
    				//close the mutex and push the close record onto queue
                		pthread_mutex_unlock(&mylock[1]);
                		sem_post(&mysem[1]);
			}//end if
		}//end else
	reccount++;
        }//end while

	//join the child threads back in 
	for (int k = 1; k < 6; k++)
	{
		pthread_join(p[k], NULL);
	}
	cout << "Order Processing is finished." << endl;
	cout << "The total dollar valued of orders processed is: $" << runningTotal << endl; 	
        close(connection);
        pthread_exit(0);
}//end handleRequest
/*****************************************************************************/
//station 0 code
//calculate tax and check for idnumber > 999
void* fnthread1(void* parameter)
{
        int prodRec = 0;
        struct product_record rec;
        while(rec.idnumber != -1)
        {
	       	sem_wait(&mysem[1]);
                pthread_mutex_lock(&mylock[1]);
                rec = myqueue[1].front();
                myqueue[1].pop();
                pthread_mutex_unlock(&mylock[1]);
                //if product idnumber is 1000 or higher, then no sANDh
                //calculate tax
                rec.tax = ((rec.number * rec.price) * .05);
                rec.stations[0]= 1;
                if (rec.idnumber != -1)
                {
                        prodRec++;
                }//end if
                //if idnumber > 999, put record on next station's queue (no sANDh)
                if (rec.idnumber > 999)
                {
                        pthread_mutex_lock(&mylock[3]);
                        myqueue[3].push(rec);
                        pthread_mutex_unlock(&mylock[3]);
                        sem_post(&mysem[3]);
                }//end if
                        else
                        {
                                pthread_mutex_lock(&mylock[2]);
                                myqueue[2].push(rec);
                                pthread_mutex_unlock(&mylock[2]);
                                sem_post(&mysem[2]);
                        }//end else
        }//end while
        cout << "Station 0 processed " << prodRec << " product records" << endl;
        pthread_exit(0);
}//end fnthread1 (station 0 code)
/*****************************************************************************/
//station 1 code
//calculate shipping and handling
void* fnthread2(void* parameter)
{
        int prodRec = 0;
        struct product_record rec;
        while(rec.idnumber != -1)
        {
                sem_wait(&mysem[2]);
                pthread_mutex_lock(&mylock[2]);
                rec = myqueue[2].front();
                myqueue[2].pop();
                pthread_mutex_unlock(&mylock[2]);
                //process the record with station code
                if (rec.idnumber != -1)
                {
                        rec.sANDh = 10 + ((rec.number * rec.price) * .01);
                        rec.stations[1]= 1;
                        prodRec++;
                }//end if
                //place record on next station's queue
                pthread_mutex_lock(&mylock[3]);
                myqueue[3].push(rec);
                pthread_mutex_unlock(&mylock[3]);
                sem_post(&mysem[3]);
        }//end while
        cout << "Station 1 processed " << prodRec << " product records" << endl;
        pthread_exit(0);
}//end fnthread2 (station 1 code)
/*****************************************************************************/
//station 2 code
//calculate order total
void* fnthread3(void* parameter)
{
        int prodRec = 0;
        struct product_record rec;
        while(rec.idnumber != -1)
        {
                sem_wait(&mysem[3]);
                pthread_mutex_lock(&mylock[3]);
                rec = myqueue[3].front();
                myqueue[3].pop();
                pthread_mutex_unlock(&mylock[3]);
                //process the record with station code
                if (rec.idnumber != -1)
                {
                        rec.stations[2]= 1;
                        rec.total = ((rec.number * rec.price) + rec.tax + rec.sANDh);
                        prodRec++;
                }//end if
                //place record on next station's queue
                pthread_mutex_lock(&mylock[4]);
                myqueue[4].push(rec);
                pthread_mutex_unlock(&mylock[4]);
                sem_post(&mysem[4]);
        }//end while
        cout << "Station 2 processed " << prodRec << " product records" << endl;
        pthread_exit(0);
}//end fnthread3 (station 2 code)
/*****************************************************************************/
//station 3 code
//calculates/totalizes running total for all orders
void* fnthread4(void* parameter)
{
        int prodRec = 0;
        struct product_record rec;
        while(rec.idnumber != -1)
        {
                sem_wait(&mysem[4]);
                pthread_mutex_lock(&mylock[4]);
                rec = myqueue[4].front();
                myqueue[4].pop();
                pthread_mutex_unlock(&mylock[4]);
                //process the record with station code
                if (rec.idnumber != -1)
                {
	                rec.stations[3]= 1;
                        runningTotal=(runningTotal + rec.total);
                        cout << "The running total of all orders seen so far is: $"
                                                << runningTotal << endl;
                        prodRec++;
                }//end if
                //place record on next station's queue
                pthread_mutex_lock(&mylock[5]);
                myqueue[5].push(rec);
                pthread_mutex_unlock(&mylock[5]);
                sem_post(&mysem[5]);
        }//end while
        cout << "Station 3 processed " << prodRec << " product records" << endl;
        pthread_exit(0);
}//end fnthread4 (station 3 code)
/*****************************************************************************/
//station 4 code
//reads record queue, display records, and enqueue to next station

void* fnthread5(void* parameter)
{
        int prodRec = 0;//number of records processed
        struct product_record rec;
        //read record queue, process records, and enqueue to next
        while(rec.idnumber != -1)
        {
                sem_wait(&mysem[5]);
                pthread_mutex_lock(&mylock[5]);
                rec = myqueue[5].front();
                myqueue[5].pop();
                pthread_mutex_unlock(&mylock[5]);
                //displays the records to the screen
                pthread_mutex_lock(&mylock[0]);
                if (rec.idnumber != -1)
                {
                        rec.stations[4]= 1;
                        cout << "Product ID: " << rec.idnumber << endl <<
                        "Product Name: " << rec.name << endl <<
                        "Price: $" << rec.price << endl <<
                        "Number Ordered: " << rec.number << endl <<
                        "Tax: $" << rec.tax << endl <<
                        "Shipping and Handling: $" << rec.sANDh << endl <<
                        "Order Total: $" << rec.total << endl <<
                        "Stations Processed: " << rec.stations[0] << " " <<
                                   rec.stations[1] << " " << rec.stations[2] << " "
                                     << rec.stations[3] << " " << rec.stations[4] << endl;
                        prodRec++;
                }//end if
                pthread_mutex_unlock(&mylock[0]);
                //place record on next station's queue
                pthread_mutex_lock(&mylock[6]);
                myqueue[6].push(rec);
                pthread_mutex_unlock(&mylock[6]);
                sem_post(&mysem[6]);
        }//end while
        cout << "Station 4 processed " << prodRec << " product records" << endl;
        pthread_exit(0);
}//end fnthread5 (station 4 code)
/*****************************************************************************/
//Writer thread function
void* fnthread6(void* parameter)
{
        int connection2;
        int sockdesc;           //used to create socket
        struct addrinfo* myinfo2;//used to create connection address
        sockdesc = socket(AF_INET, SOCK_STREAM, 0);
        if ( sockdesc < 0 )
        {
                cout << "S2 Error creating socket" << endl;
                exit(0);
        }
        // Set up the address record
        if ( getaddrinfo(hostname2, portnum2, NULL, &myinfo2 ) != 0 )

        {
                cout << "S2 Error getting address" << endl;
                exit(0);
        }
/*********************************************************************/
        // Connect to the host
        connection2 = connect(sockdesc, myinfo2->ai_addr, myinfo2->ai_addrlen);
        if ( connection2 < 0 )
        {
                cout << "S2: Error in connect" << endl;
                exit(0);
        }
/**************************************************************************/
        struct thread_args fn_args;
        int value;
        //read the records and place on next queue
	myProductAft.idnumber = 0;
        while(myProductAft.idnumber != -1)
        {
                  sem_wait(&mysem[6]);
                  pthread_mutex_lock(&mylock[6]);
                  myProductAft = myqueue[6].front();
                  myqueue[6].pop();
		  pthread_mutex_unlock(&mylock[6]);
        send(sockdesc, (void*)&myProductAft, sizeof(product_record), 0);
        }
        close(sockdesc);
   	pthread_exit(0);
}//end thread
/*****************************************************************************/

















































