#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <err.h>
#include <stdint.h>
#include <errno.h>

#include <signal.h>
#include <syslog.h>
#include <sys/wait.h>

/*
 programmed by Artem Kolpakov
*/

// Error function used for reporting issues
void error(const char *msg) {
  perror(msg);
  exit(1);
} 

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber){
 
  memset((char*) address, '\0', sizeof(*address));  // Clear out the address struct

  address->sin_family = AF_INET;                    // The address should be network capable
  address->sin_port = htons(portNumber);            // Store the port number
  address->sin_addr.s_addr = INADDR_ANY;            // Allow a client at any address to connect to this server
}

//CITATION: Chapter 60.3 The Linux Programming Interface
static void grimReaper(){
    int savedErrno;
    /* Save 'errno' in case changed here */
    savedErrno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0){ //reap all dead child processes
      continue;
    }    
    errno = savedErrno;
}

// YOU CAN UNCOMMENT ALL THE PRINTFs TO TEST THE CLIENT-SEVER INTERACTION FLOW
int main(int argc, char *argv[]){
  int connectionSocket, charsRead;
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);
  char buffer[256];

  // Check usage & args
  if (argc < 2) { 
    fprintf(stderr,"USAGE: %s <port>\n", argv[0]);      //check if port wasn't provided
    exit(1);
  } 
  
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);       // Create the socket that will listen for connections
  if (listenSocket < 0) {
    error("ERROR opening socket");
  }

  setupAddressStruct(&serverAddress, atoi(argv[1]));        // Set up the address struct for the server socket

  // Associate the socket to the port
  if (bind(listenSocket, 
          (struct sockaddr *)&serverAddress, 
          sizeof(serverAddress)) < 0){
    error("ERROR on binding");
  }
  listen(listenSocket, 5);          // Start listening for connetions. Allow up to 5 connections to queue up
  
  // Accept a connection, blocking if one is not available until one connects
  while(1){
    grimReaper();   // reap dead child processes (connections) if there are any
    // Accept the connection request which creates a connection socket
    connectionSocket = accept(listenSocket, 
                (struct sockaddr *)&clientAddress, 
                &sizeOfClientInfo); 
    if (connectionSocket < 0){
      error("ERROR on accept");
    }

    // printf("SERVER: Connected to client running at host %d port %d\n", ntohs(clientAddress.sin_addr.s_addr), ntohs(clientAddress.sin_port));

    // CITATION: the logic and structure of forking has been adapted from the Chapter 60.3 of The Linux Programming Interface
    switch (fork()) {
      case -1:    //fail
        syslog(LOG_ERR, "Can't create child (%s)", strerror(errno));
        break;                        // May be temporary; try next client
      case 0:     //child
        memset(buffer, '\0', 256);
        charsRead = recv(connectionSocket, buffer, 255, 0);     //get first test message 't'
        if (charsRead < 0){
          error("ERROR reading from socket");
        }
        //printf("SERVER: I received this from the client: \"%s\"\n", buffer);
        if(buffer[0] == 't'){   //if we got a test message from client
          charsRead = send(connectionSocket, "t", 1, 0);     // Send a Success message back to the client
          if (charsRead < 0){
            error("ERROR reading from socket");
          }
          //start reading key + text from socket
          /*---------------------------------------------------------------------------------------------------*/
          //read key to use it for encryption
          int keyBufferLength = 0;
          memset(buffer, '\0', 256);
          charsRead = recv(connectionSocket, buffer, 255, 0);   //get a key length
          keyBufferLength = atoi(buffer);      //string to integer
          // printf("SERVER: READ key buff length: %d\n",  keyBufferLength); //test
          char *keyBuffer = malloc(keyBufferLength + 1);              //allocate memory for keyBuffer string
          memset(keyBuffer, '\0', sizeof(keyBuffer));
          int startFrom = 0;
          //start reading the entire key, 1k chars at a time
          while (keyBufferLength > 0){    
            charsRead = recv(connectionSocket, keyBuffer+startFrom, 1000, 0);    //read 1k chars at a time
            if (charsRead < 0){
              error("CLIENT: ERROR reading from socket");
            }
            keyBufferLength = keyBufferLength - charsRead;  //decrement length
            // if (charsRead < 1000 && keyBufferLength > 0){
            //   printf("CLIENT: WARNING: Not all data read from the socket!\n");
            // }
            startFrom = startFrom + charsRead;          //update a pointer to start reading from 
          }
          // printf("SERVER: READ key buff: %s\n",  keyBuffer); //test
          send(connectionSocket, "s", 1, 0);              //send success message that we have read the key
          
          /*---------------------------------------------------------------------------------------------------*/
          //read text to encrypt it
          int plaintextBufferLength = 0;
          memset(buffer, '\0', 256);
          charsRead = recv(connectionSocket, buffer, 255, 0);     //start with reading a length of the plaintext
          if (charsRead < 0){
            error("CLIENT: ERROR reading from socket");
          }
          plaintextBufferLength = atoi(buffer);       //string to integer
          // printf("SERVER: READ text buff length: %d\n", plaintextBufferLength); //test
          send(connectionSocket, "s", 1, 0);          //send success message that we have read the plaintext length

          char *plaintextBuffer = malloc(plaintextBufferLength + 1);             //allocate memory for plaintextBuffer
          memset(plaintextBuffer, '\0', sizeof(plaintextBuffer));
          startFrom = 0;
          //start reading the entire plaintext, 1k chars at a time
          while (plaintextBufferLength > 0){    
            charsRead = recv(connectionSocket, plaintextBuffer+startFrom, 1000, 0);          //read 1k chars at a time
            if (charsRead < 0){
              error("CLIENT: ERROR reading from socket");
            }
            plaintextBufferLength = plaintextBufferLength - charsRead;    //decrement the length left to read by chars read
            // if (charsRead < 1000 && plaintextBufferLength > 0){
            //   printf("SERVER: WARNING: Not all data read from the socket!\n");
            // }
            startFrom = startFrom + charsRead;          //update a pointer to start reading from 
          }
          // printf("SERVER: READ text buff: %s\n", plaintextBuffer); //test
          // printf("SERVER: READ key buff: %s\n", keyBuffer); //test
          /*---------------------------------------------------------------------------------------------------*/
          //having read plaintext and key perform the actual encryption on plaintext
          for(int i = 0; i < strlen(plaintextBuffer); i++){
            // printf("plaintextBuffer[i]: %c \n", plaintextBuffer[i]);
            // printf("keyBuffer[i]: %c \n", keyBuffer[i]);

            //convert to ASCI and perform (message + key) mod 26+1 (because of the ' ')
            // ' ' will give wrong int when converting to ascii so we have to replace it with 26 to get a perfect ' ' back
            if(plaintextBuffer[i] == ' ' && keyBuffer[i] == ' '){   //if both are ' '
              plaintextBuffer[i] = (26 + 26) % 27;        //use 26 instead of ' '
            } else if (plaintextBuffer[i] == ' ' && keyBuffer[i] != ' '){ //if one is ' '
              plaintextBuffer[i] = (26 + (keyBuffer[i]-65)) % 27;
            } else if (plaintextBuffer[i] != ' ' && keyBuffer[i] == ' '){ //if one is ' '
              plaintextBuffer[i] = ((plaintextBuffer[i]-65) + 26) % 27;
            } else if (plaintextBuffer[i] != ' ' && keyBuffer[i] != ' '){ //if neither is ' ' then perform the conversion as it should be
              plaintextBuffer[i] = ((plaintextBuffer[i]-65) + (keyBuffer[i]-65)) % 27;
            }

            //convert back
            if (plaintextBuffer[i] == 26){
              plaintextBuffer[i] = ' ';
            }
            else{   //do normal conversion
              plaintextBuffer[i] = plaintextBuffer[i] + 65;
            }
          }
          
          // printf("SERVER: ENCODED text buff: %s\n", plaintextBuffer); //test
          /*---------------------------------------------------------------------------------------------------*/
          //send encrypted data back to client
          send(connectionSocket, "r", 1, 0);              //send "ready" message that we have encoded the text and are ready to send it back
          //start sending the encrypted data back
          //send length of encrypted data
          plaintextBufferLength = strlen(plaintextBuffer);      //get length for allocation of stringPlaintextBufferLength string
          char *stringPlaintextBufferLength = malloc(plaintextBufferLength + 1);              //allocate memory for stringPlaintextBufferLength
          snprintf(stringPlaintextBufferLength, plaintextBufferLength + 1, "%d", plaintextBufferLength); //formats and stores int plaintextBufferLength into str stringPlaintextBufferLength
          // fprintf(stderr, "Converted plaintextBufferLength %d to stringPlaintextBufferLength %s \n", plaintextBufferLength, stringPlaintextBufferLength);  //test
          send(connectionSocket, stringPlaintextBufferLength, sizeof(stringPlaintextBufferLength), 0);  // Send the buffer size to the server
          // fprintf(stderr, "SERVER: sent size of encrypted text (%s) \n", stringPlaintextBufferLength); //test

          char testBuffer[2];
          memset(testBuffer, '\0', sizeof(testBuffer));         // Clear out the buffer array      

          charsRead = recv(connectionSocket, testBuffer, 1, 0); // check if client gets it (the final result's size)
          if (charsRead < 0){
            error("SERVER: ERROR reading from socket, server connection failed!");
          }

          if (testBuffer[0] == 's') {          //client sends back "s" if it gets the encrypted data length 
            //Send the actual encrypted data 1k chars at a time
            startFrom = 0;
            int charsWritten = 0;
            while (plaintextBufferLength > 0){
              charsWritten = send(connectionSocket, plaintextBuffer + startFrom, 1000, 0);      //send 1k chars at a time
              if (charsWritten < 0){
                error("SERVER: ERROR writing to socket");
              }
              startFrom = startFrom + charsWritten;          //update a pointer to start writing from 
              plaintextBufferLength = plaintextBufferLength - charsWritten;    //decrement num of chars written 
              if (charsWritten < 1000 && plaintextBufferLength > 0){
                printf("SERVER: WARNING: Not all data written to socket!\n");
              }
            }
            // fprintf(stderr, "SERVER: sent the encrypted data \n"); //test
          } 
          else {    //if client does not get the length of encrypted data
            err(errno, "SERVER: Failure! Server connection failed when sending encrypted data length!");
          }
          close(connectionSocket);            // Close the connection socket for this client
        }
        else{     //if we didn't get test message from client
          send(connectionSocket, "f", 1, 0);  // Send the indication of fail, do that to only have 1 error when file has invalid data for text to get encrypted
          close(connectionSocket);            // Close the connection socket for this client
        }
      default:    // Parent
        close(connectionSocket);              // Unneeded copy of connected socket
        int savedErrno;
        savedErrno = errno;
        while (waitpid(-1, NULL, WNOHANG) > 0){   //reap all dead child processes
          continue;
        }
        errno = savedErrno;
        break;
    }
  }
  close(listenSocket);      // Close the listening socket
  return 0;
}
