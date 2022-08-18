#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()
#include <errno.h>
#include <err.h>
#include <stdint.h>

/*
 programmed by Artem Kolpakov
*/

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Read key + data to get encrypted from files and send that input as a message to the server.
* 3. Print the encrypted message received back from the server and exit the program.
*/

// YOU CAN UNCOMMENT ALL THE PRINTFs TO TEST THE CLIENT-SERVER INTERACTION FLOW
static char const alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

// Error function used for reporting issues
void error(const char *msg) { 
  perror(msg); 
  exit(0); 
} 

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber, 
                        char* hostname){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname(hostname); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

int main(int argc, char *argv[]) {
  int socketFD, portNumber, charsWritten, charsRead;
  struct sockaddr_in serverAddress;
  // Check usage & args
  if (argc < 4) {       //check of there are less than 3 args provided 
    fprintf(stderr,"USAGE: %s <plaintext> <key> <port>\n", argv[0]); 
    exit(0); 
  } 

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    error("CLIENT: ERROR opening socket");
  }

   // Set up the server address struct, pass port number, our 3rd argument
  setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

  // read plaintext to encrypt from the file
  FILE* source1 = fopen(argv[1], "r");
  if(source1 == NULL) {              // check if file opening fails
    err(errno, "fopen()");
  }

  char* plaintextBuffer = NULL;     // create a buffer to hold plaintext
  size_t length = 0;
  size_t plaintextCharsRead = getline(&plaintextBuffer, &length, source1);    //read the line to fill the plaintext buff
  if(plaintextCharsRead == -1){     //check for error when reading the line
    err(errno, "getline()");
  }

  // read key from the file
  FILE* source2 = fopen(argv[2], "r");
  if(source2 == NULL) {              // check if file opening fails
    err(errno, "fopen()");
  }

  char *keyBuffer = NULL;         // create a buffer to hold key
  size_t lengthG = 0;
  size_t keyCharsRead = getline(&keyBuffer, &lengthG, source2);      //read the line to fill the key buff
  if(keyCharsRead == -1){       //check for error when reading the line
    err(errno, "getline()");
  }

  /*------------------------------------------------------------------------------------------------------------*/
  // If enc_client receives key or plaintext files with ANY bad characters in them, or the key file is shorter 
  // than the plaintext, then it terminates, sends appropriate error text to stderr, and sets the exit value to 1.

  //check if key is smaller than text to get encrypted
  if(plaintextCharsRead > keyCharsRead){
    fprintf(stderr, "ERROR: %s provide longer <key> \n", argv[0]); 
    exit(1);
  }

  plaintextBuffer[strcspn(plaintextBuffer, "\n")] = '\0';  //cut a \n
  //check for problematic chars that shouldn't be on the plaintext that we read
  for(int i = 0; i < plaintextCharsRead; i++){
    // strchr returns a pointer to the first occurrence of the character plaintextBuffer[i] in the allowed alphabet
    char* occur = strchr(alphabet, plaintextBuffer[i]);
    if (!occur){      //if plaintextBuffer[i] is not in the allowed range
      // fprintf(stderr, "p: %c \n", plaintextBuffer[i]); 
      fprintf(stderr, "ERROR: %s <plaintext> file has invalid characters in it! \n", argv[0]); 
      exit(1);
    }
  }

  keyBuffer[strcspn(keyBuffer, "\n")] = '\0';  //cut a \n
  for(int i = 0; i < keyCharsRead; i++){
    // strchr returns a pointer to the first occurrence of the character plaintextBuffer[i] in the allowed alphabet
    char* occur = strchr(alphabet, keyBuffer[i]);
    if (!occur){      //if plaintextBuffer[i] is not in the allowed range
      // fprintf(stderr, "p: %c \n", plaintextBuffer[i]); 
      fprintf(stderr, "ERROR: %s <key> file has invalid characters in it! \n", argv[0]); 
      exit(1);
    }
  }

  //check for problematic chars that shouldn't be on the key that we read
  for(int i = 0; i < keyCharsRead; i++){
    // strchr returns a pointer to the first occurrence of the character keyBuffer[i] in the allowed alphabet
    char* occur = strchr(alphabet, keyBuffer[i]);
    if (!occur){      //if keyBuffer[i] never occurs in alphabet then it's an invalid char
      fprintf(stderr, "ERROR: %s <key> file has invalid characters in it! \n", argv[0]);
      exit(1);
    }
  }
  
  /*------------------------------------------------------------------------------------------------------------*/
  //start working with a server by sending a test message to establish connection
  //After we made sure that the data we are sending is read and is correct, attempt to connect to server
  //if enc_client cannot connect to the enc_server server, for any reason (including that it has accidentally tried to connect to the dec_server server), 
  //it reports this error to stderr with the attempted port, and set the exit value to 2.
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: ERROR connecting");
  }

  charsWritten = send(socketFD, "t", 1, 0);  // Write test to the server, send free trial text message to server
  if (charsWritten < 0){
    error("CLIENT: ERROR writing to socket, server connection failed!");
  }

  char testBuffer[2];
  memset(testBuffer, '\0', sizeof(testBuffer));         // Clear out the buffer array   
   // Get return message from server
  charsRead = recv(socketFD, testBuffer, 1, 0);         // Get letter t back from the server if success
  if (charsRead < 0){
    error("CLIENT: ERROR reading from socket, server connection failed!");
  }

  if (testBuffer[0] == 't') {          //we send back t if connection succeeds
    // printf("Success! Server is connected and ready for exploitation! \n");
  }
  else {
    if (testBuffer[0] == 'f'){
      fprintf(stderr, "Failure! Server connection failed! Could not contact %s on port %d \n", argv[0], atoi(argv[3]));
      exit(2);
    }
  }
  
  /*------------------------------------------------------------------------------------------------------------*/
  //knowing that the server works, now lets start sending the data we have read from files
  //fist send the key 
  //start with sending the length of key (so that we have an understanding how much data to read when we are gonna be reading it on the server side)

  int keyBufferLength = strlen(keyBuffer);      //get length for allocation of keyBufferLength string
  char *stringkeyBufferLength = malloc(keyBufferLength + 1);              //allocate memory for stringkeyBufferLength
  snprintf(stringkeyBufferLength, keyBufferLength + 1, "%d", keyBufferLength);  //formats and stores int keyBufferLength into str stringkeyBufferLength
  // fprintf(stderr, "Converted keyBufferLength %d to stringkeyBufferLength %s \n", keyBufferLength, stringkeyBufferLength);  //test
  send(socketFD, stringkeyBufferLength, sizeof(stringkeyBufferLength), 0);  // Send the buffer size to the server
  // fprintf(stderr, "CLIENT: sent size of key (%s) \n", stringkeyBufferLength);  //test

  //now send a keyBuffer itself, 1k chars at a time
  keyBuffer[strcspn(keyBuffer, "\n")] = '\0';  //cut a \n
  char* buffer = (char*) calloc(strlen(keyBuffer), sizeof(char));    //allocate dynamic memory to hold the key
  memset(buffer, '\0', sizeof(buffer));
  strcpy(buffer, keyBuffer);
  
  int startFrom = 0;
  //Since we are supposed to break the transmission every 1000 characters, we'll need to wrap send process in a loop
  while (keyBufferLength > 0){
    charsWritten = send(socketFD, buffer + startFrom, 1000, 0);      //send 1k chars at a time
    if (charsWritten < 0){
      error("CLIENT: ERROR writing to socket");
    }
    startFrom = startFrom + charsWritten;           //update a pointer to start writing from (in the next iteration)
    keyBufferLength = keyBufferLength - charsWritten;    //decrement num of chars written 
    if (charsWritten < 1000 && keyBufferLength > 0){     //check if we wrote less than 1k when we had more than 1k left
      fprintf(stderr, "CLIENT: WARNING: Not all data written to socket!\n"); 
    }
  }
  // fprintf(stderr, "CLIENT: sent the key \n"); //test
  free(buffer);
  /*------------------------------------------------------------------------------------------------------------*/
  //now send the plaintext 
  memset(testBuffer, '\0', sizeof(testBuffer));         // Clear out the buffer array                              
  charsRead = recv(socketFD, testBuffer, 1, 0);
  if (charsRead < 0){
    error("CLIENT: ERROR reading from socket, server connection failed!");
  }
  if (testBuffer[0] == 's') {          //we send receive "s" if reading keytext on the server side was successful
    // printf("Success! The key has been read, proceed to writing the text! \n");

    // start with sending the length of plaintext (so that have an understanding how much data to read when we are gonna be reading it on server side)
    int plaintextBufferLength = strlen(plaintextBuffer);        //get length for allocation of keyBufferLength string
    char *stringPlaintextBufferLength = malloc(plaintextBufferLength + 1);              //allocate memory for stringPlaintextBufferLength
    snprintf(stringPlaintextBufferLength, plaintextBufferLength + 1, "%d", plaintextBufferLength); //formats and stores int keyBufferLength into str stringkeyBufferLength
    // fprintf(stderr, "Converted plaintextBufferLength %d to stringPlaintextBufferLength %s \n", plaintextBufferLength, stringPlaintextBufferLength);  //test
    send(socketFD, stringPlaintextBufferLength, sizeof(stringPlaintextBufferLength), 0); // Send the buffer size to the server
    // fprintf(stderr, "CLIENT: sent size of text (%s) \n", stringPlaintextBufferLength); //test 

    //now send a plaintext itself
    plaintextBuffer[strcspn(plaintextBuffer, "\n")] = '\0';  //cut a \n
    char* buffer1 = (char*) calloc(strlen(plaintextBuffer), sizeof(char));    //allocate dynamic memory to hold the text
    memset(buffer1, '\0', sizeof(buffer));
    strcpy(buffer1, plaintextBuffer);

    memset(testBuffer, '\0', sizeof(testBuffer));         // Clear out the buffer array                              
    charsRead = recv(socketFD, testBuffer, 1, 0);         // check if server recieved our buffer length
    if (charsRead < 0){
      error("CLIENT: ERROR reading from socket, server connection failed!");
    }
    if (testBuffer[0] == 's') {          //we recieve 's' back if plaintext length has been successfully read
      // printf("Success! The plaintextBufferLength length has been read, proceed to writing the plaintext itseld! \n");
      startFrom = 0;
      //Since we are supposed to break the transmission every 1000 characters, we'll need to wrap send process in a loop
      while (plaintextBufferLength > 0){
        charsWritten = send(socketFD, buffer1 + startFrom, 1000, 0);      //send 1k chars at a time
        if (charsWritten < 0){
          error("CLIENT: ERROR writing to socket");
        }
        startFrom = startFrom + charsWritten;           //update a pointer to start writing from 
        plaintextBufferLength = plaintextBufferLength - charsWritten;    //decrement num of chars written 
        if (charsWritten < 1000 && plaintextBufferLength > 0){           //check if we wrote less than 1k when we had more than 1k left
          fprintf(stderr, "CLIENT: WARNING: Not all data written to socket!\n");
        }
      }
      // fprintf(stderr, "CLIENT: sent the text \n"); //test
    }
    if (charsRead < 0){
      error("CLIENT: ERROR reading from socket, server connection failed!");
    }
    free(buffer1);       // deallocate the dynamic memory occupied by buffer
  }
  else {    //if we don't get 's' back, then serv connection failed, as reading keytext failed
    err(errno, "CLIENT: Failure! Reading key length failed!");
  }

  /*------------------------------------------------------------------------------------------------------------*/
  // Get return message from server and print it
  memset(testBuffer, '\0', sizeof(testBuffer));         // Clear out the buffer array                              
  charsRead = recv(socketFD, testBuffer, 1, 0);
  if (charsRead < 0){
    error("CLIENT: ERROR reading from socket, server connection failed!");
  }
  if (testBuffer[0] == 'r') {           //serv sends back r if it has encoded the text and ready to send it
      //read length of encrypted data
      int EncPlaintextBufferLength = 0;
      memset(buffer, '\0', 256);
      charsRead = recv(socketFD, buffer, 255, 0);   //get the length of encrypted data to create a buff to hold it
      if (charsRead < 0){
        error("CLIENT: ERROR reading from socket");
      }
      EncPlaintextBufferLength = atoi(buffer);      //string to integer
      // fprintf(stderr, "CLIENT: READ encrypted text buff length: %d\n", EncPlaintextBufferLength); //test
      send(socketFD, "s", 1, 0);             // send success message that we have read the encrypted data
      //read encrypted data 
      char *EncTextBuffer = malloc(EncPlaintextBufferLength + 1);              //allocate memory for buffer to hold result
      memset(EncTextBuffer, '\0', sizeof(EncTextBuffer));
      startFrom = 0;
      //start reading the encrypted data 1k chars at a time
      while (EncPlaintextBufferLength > 0){    
        charsRead = recv(socketFD, EncTextBuffer+startFrom, 1000, 0);          //read 1k chars at a time
        if (charsRead < 0){
          error("CLIENT: ERROR reading from socket");
        }
        EncPlaintextBufferLength = EncPlaintextBufferLength - charsRead;    //decrement the length
        startFrom = startFrom + charsRead;          //update a pointer to start reading from 
      }
      //print encrypted data

      // fprintf(stderr, "CLIENT: READ encrypted text: %s\n", EncTextBuffer); //test
      printf("%s\n", EncTextBuffer);    //send the result to stdout, add \n too
      fflush(stdout);         //flush out the contents of an output stream
  }
  else {    //something went wrong with server's encoding process
    err(errno, "Failure! Reading encrypted data failed!");
  }

  close(socketFD);            // Close the socket
  return 0;
}
