///////////////////////////////////////////////////////////////////////////////
//Nathan Timbs CSCI 4727 4/24/10 prog5client.cc
//This program acts as a client. It attempts to establish a socket connection
//with a server program which is already running. It opens a file, extracts
//records, and sends them one at a time for processing to the server. A thread 
//is created to establish another socket. This thread uses the socket to 
//receive records processed by the server and places them in an output file. 
//This program accepts 5 command line parameters: input file, output file, 
//server name, and server port number. The program sends the first record 
//to the server with port and host name info for the second socket connection.
///////////////////////////////////////////////////////////////////////////////
#include <fstream>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#define MAXRECORDS 1000		//Maximum number of records in an input file
#define MAXSTAGES 5		//Number of processing stations
#define PRODUCTSIZE 128		//Product Name buffer size
int reccount = 0;
using namespace std;
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
product_record myProductArr[MAXRECORDS];//used to read from file and to children
product_record myProductAft[MAXRECORDS];
product_record myProduct;		//Holds unprocessed record
product_record myProductAfter;		//Holds processed record
void get_info(char*);			//function to retrieve records from file
void put_info(char*);			//function to store records into file
void* handleRead(void* arg);
void* handleSend(void* arg);
char hostname2[81];
char portnum2[81];
char* outFile;
pthread_t thrd;
pthread_t thrd2;
int sockdesc2;
int connection2;
struct addrinfo* myinfo2;//used to create connection

int main(int argc, char** argv)
{
        char* inFile;   	//input file
	struct addrinfo* myinfo;//used to create connection
	int sockdesc;		//used to create socket
   	char hostname[81];	//host name or address
   	char portnum[81];	//host port number
   	char buffer[81];	
   	int connection;
   	int value;

        inFile = argv[1];
        outFile = argv[2];
	strcpy(hostname, argv[3]);
	strcpy(portnum, argv[4]);	
	strcpy(portnum2, argv[5]);//2nd connection port number
//	strcpy(hostname2, argv[3]);
	strcpy(hostname2, "einstein.etsu.edu");
	get_info(inFile);//retrieve records from file

/********************************************************************/
//Initialize Connection for return records
/********************************************************************/
        // Use SOCK_DGRAM for UDP datagrams
        sockdesc2 = socket(AF_INET, SOCK_STREAM, 0);
        if ( sockdesc2 < 0 )
        {
                cout << "C2: Error creating socket" << endl;
                exit(0);
        }

        if ( getaddrinfo( "0.0.0.0", portnum2, NULL, &myinfo2) != 0 )
        {
                cout << "C2: Error getting address" << endl;
                exit(0);
        }
        // Bind the socket to an address
        if (bind(sockdesc2, myinfo2->ai_addr, myinfo2->ai_addrlen) < 0 )
        {
                cout << "C2 Error binding to socket" << endl;
                exit(0);
        }
        // Now listen to the socket
        if ( listen(sockdesc2, 1) < 0 )
        {
                cout << "C2 Error in listen" << endl;
                exit(0);
        }

/********************************************************************/
//Create Client Socket
/********************************************************************/
	// Use AF_UNIX for unix pathnames instead
   	// Use SOCK_DGRAM for UDP datagrams
   	sockdesc = socket(AF_INET, SOCK_STREAM, 0);
   	if ( sockdesc < 0 )
   	{
      		cout << "C1: Error creating socket" << endl;
      		exit(0);
   	}

   	if ( getaddrinfo( hostname, portnum, NULL, &myinfo) != 0 )
   	{
      		cout << "C1: Error getting address" << endl;
      		exit(0);
   	}

   	// Connect to the host
   	connection = connect(sockdesc, myinfo->ai_addr, myinfo->ai_addrlen);
   	if ( connection < 0 )
   	{
      		cout << "C1: Error in connect" << endl;
      		exit(0);
   	}
/**********************************************************************/
        //Create thread for writing to file
        pthread_create(&thrd, NULL, handleRead, (void *)&connection);
        cout << "Client: Writer thread function has been called" << endl;

	product_record myProduct;
	for (int x =0; x<reccount; x++)
	{ 
                myProduct= myProductArr[x];
    		//Send a record	
    		value = send(sockdesc, (void*)&myProduct, sizeof(product_record), 0);
   	}//end for
	//close the socket
	close(sockdesc);
	pthread_join( thrd, NULL);
   	return 0;
}//end main
/*************************Retrieve records from file*************************/
void get_info(char* inFile)
{
//Read in input file and fill array of structs
        FILE * pFile;
        char linebuf[128];
        char str[128];
        char * item;
        int i;
        int j;
        pFile = fopen (inFile,"r");
	
	//encode first record with hostname and portnum for server writing thread
	myProductArr[reccount].idnumber = atoi(portnum2);
	sscanf(hostname2, "%s", myProductArr[reccount].name, &i);

	reccount++;

        if (pFile==NULL)
        {
                perror ("Error opening file");
        }
        else//read in file to structs
	{	
                while (!feof(pFile))
                {
                        if (feof(pFile))
                        {
                                break;
                        }
                        //get id number
                        fgets(linebuf,128,pFile);
                        myProductArr[reccount].idnumber = atoi(linebuf);
                        if (feof(pFile))
                        {
                                break;
                        }
                        //get name
                        fgets(linebuf,128,pFile);
                        sscanf(linebuf, "%s", myProductArr[reccount].name, &i);
                        if (feof(pFile))
                        {
                                break;
                        }
                        //get price
                        fgets(linebuf,128,pFile);
                        myProductArr[reccount].price = atof(linebuf);
                        if (feof(pFile))
                        {
                                break;
                        }
                        //get number ordered
                        fgets(linebuf,128,pFile);
                        myProductArr[reccount].number = atoi(linebuf);
                        if (feof(pFile))
                        {
                                break;
                        }
                        //get tax
                        fgets(linebuf,128,pFile);
                        myProductArr[reccount].tax = atof(linebuf);
                        if (feof(pFile))
                        {
                                break;
                        }
                        //get shipping and handling
                        fgets(linebuf,128,pFile);
                        myProductArr[reccount].sANDh = atof(linebuf);
                        if (feof(pFile))
                        {
                                break;
                        }
                        //get order total
                        fgets(linebuf,128,pFile);
                        myProductArr[reccount].total = atof(linebuf);
                        if (feof(pFile))
                        {
                                break;
                        }
                        //get stations
                        fgets(linebuf,128,pFile);
                        strcpy (linebuf,"0\n");
                        myProductArr[reccount].stations[atoi(linebuf)];
			if (myProductArr[reccount].idnumber != -1)
			{
				reccount++; //check for idnumbers of -1, if found discard
			}              	
		        if (feof(pFile))
                        {
                                break;
                        }
                }//end while
	        fclose(pFile);
        	//generate last record which tells children to exit
                int q;
                myProductArr[reccount].idnumber= -1;
                sscanf(linebuf, "%s", myProductArr[reccount].name, &q);
                myProductArr[reccount].price= 0.0;
                myProductArr[reccount].number= 0;
                myProductArr[reccount].tax= 0.0;
                myProductArr[reccount].sANDh= 0.0;
                myProductArr[reccount].total= 0.0;
                myProductArr[reccount].stations[0]= 0;
                reccount++;
        }//end else
}//end get_info
/**************************put array into file********************************/
void put_info(char* outFile)
{
int readcount = reccount-1;
int m;
ofstream myfile;
myfile.open (outFile);
        for (m=0; m < (readcount-1); m++)
        {
                myfile << myProductAft[m].idnumber << "\n";
                myfile << myProductAft[m].name << "\n";
                myfile << myProductAft[m].price << "\n";
                myfile << myProductAft[m].number << "\n";
                myfile << myProductAft[m].tax << "\n";
                myfile << myProductAft[m].sANDh << "\n";
                myfile << myProductAft[m].total << "\n";
                myfile << myProductAft[m].stations[0] << " " << myProductAft[m].stations[1] << " " <<
                                myProductAft[m].stations[2] << " " << myProductAft[m].stations[3] << " " <<
                                myProductAft[m].stations[4] << "\n";
        }//end for
                myfile.close();
}//end put_info
/******************************************************************************/
void* handleRead(void* arg)
{
   	struct product_record myProductAfter;
	for(;;)
	{
    		connection2 = accept(sockdesc2, NULL, NULL);
        	if ( connection2 < 0 )
        	{
        	        cout << "C2 Error in accept" << endl;
        	        exit(0);
        	}//end if
		else
        	{//deal with the request
        	while(myProductAfter.idnumber !=-1)
        	{
        	        for (int x=0; x<reccount; x++)
        	        {
                		read(connection2, (void*)&myProductAfter, sizeof(product_record));
                		myProductAft[x]= myProductAfter;
                	}//end for
        	}//end while
       		//close the socket
        	close(connection2);
        	put_info(outFile);
        	cout << "The parent processed " << (reccount-2) << " product records." << endl;
        	pthread_exit(0);
		}  //end else
	}//end for
}//end thread

