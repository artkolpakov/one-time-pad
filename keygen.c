#include <stdlib.h>             //rand, rand_r, srand - pseudo-random number generator
#include <time.h>               //time(NULL)
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

/*
 programmed by Artem Kolpakov
*/

//The characters in the file generated will be any of the 27 allowed characters
static char const alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
                               //0 --------------------- 26//
int main(int argc, char *argv[]){
    if (argc != 2) {            //if there is more/less than 1 arguments provided
        fprintf(stderr, "Usage: %s <keylength>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));                      //generating a pseudo-random random seed
    int keylength;                          //length of the key file in characters
    keylength = atoi(argv[1]);              //converting string argument[1] to  integer
    char mykey[keylength + 1];              //+1 because of the newline
    memset(mykey, '\0', keylength+1);       //initializing key with null terminators

    for (int i = 0; i < keylength+1; i++) {     //filling key char array with random chars
        if(i != keylength){
            mykey[i] = alphabet[random() % 27];   //0-26, generate a random char out of the 27 allowed characters declared in the static alphabet array
        } else {
            mykey[i] = '\n';                      //add a newline if it's the last char in key
        }
    }
    printf("%s", mykey);    //will be used with redirecting stdout to a file, write the generated key
    fflush(stdout);         //flush out the contents of an output stream

    exit(EXIT_SUCCESS);
}
