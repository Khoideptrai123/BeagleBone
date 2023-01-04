#include "udp.h"
#include "sampler.h"
#include "circularbuffer.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h> //for close()
#include <math.h>
#include <pthread.h>
#include <string.h> //for strncmp()
#include <netdb.h> //for macros

#define MAX_LEN 1500
#define PORT 12345

#define HELP_MENU   "Accepted commands:\n\
help -- display all available commands.\n\
count -- display total number of samples taken.\n\
get N -- display the N most recent history values.\n\
length -- display number of samples in history (both max, and current).\n\
history -- display the full sample history being saved.\n\
dips -- display number of dips.\n\
stop -- cause the server program to end.\n\
<enter> -- repeat last command."

// for close purposes
static int socketDescriptor;
static pthread_t Udp_thread;
static bool stopProgram = false; // signal to main to close program, join threads and free memory

void Udp_setup(){
    printf("Leo and Quyen's Net Listen on UDP port %d: \n", PORT);
    printf("Connect using: \n");
    printf("    netcat -u 192.168.7.2 %d\n", PORT);
    //address structure
    struct sockaddr_in sin; //_in means internet
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET; //connection may be from 
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT);
    //ntonl = host network long; htons = host network short

    //Creat and bind to socket
    socketDescriptor = socket(PF_INET, SOCK_DGRAM,0);
    bind(socketDescriptor, (struct sockaddr*)&sin, sizeof(sin));
}

void Udp_establishConnection(void) {
    // printf("Spawning new UDP thread\n");
    Udp_setup();
    pthread_create(&Udp_thread, NULL, Udp_threadCommandHandling, NULL);
}


void Udp_transmitReply(size_t sin_len, char *messageTx, struct sockaddr *sinRemote) { //Send reply
    sendto(socketDescriptor,
        messageTx, strlen(messageTx),
        0,
        sinRemote, sin_len);
}

void Udp_CloseConnection(void) {
    close(socketDescriptor);
    pthread_join(Udp_thread, NULL);
}

bool Udp_signifyCloseOfProgram(void){
    return stopProgram;
}

void *Udp_threadCommandHandling(void* buffer){
    bool run = true; //boolean to keep sending the data

    // Each of the entry from history required 4 char slots for numbers
    // + 1 slot for decimal symbol (.) + 1 slot for comma + 1 slot for whitespace
    // => Each entry in the history needs 7 char slot.
    const int SLOTS_PER_ENTRY = 7;

    // Var to record the latest command
    // Initially set to help since it could help user 
    // figure out what to do
    char last_command[MAX_LEN] = "help\n";

    while(run){
        //get the data (blocking)
        //Will change sin (the address) to be the address of the client
        //Note: sin passes information in and out of call!
        struct sockaddr_in sinRemote;
        unsigned int sin_len = sizeof(sinRemote);
        char messageRx[MAX_LEN];
        //client's data writtem into messageRx string
        //sinRemote is output parameter
        //sinLen is in/out parameter
        int bytesRx = recvfrom(socketDescriptor, messageRx, MAX_LEN,0,
                            (struct sockaddr*)&sinRemote, &sin_len);
        //Null terminated (string) 
        int terminateIdx = (bytesRx < MAX_LEN)? bytesRx:MAX_LEN-1;
        messageRx[terminateIdx] =0;
        printf("Message received(%d bytes):'%s'\n", bytesRx, messageRx);

        // Compose reply message
        // (NOTE: watch for buffer overflows!)
        // If only enter was pressed => Changed tok to latest command
        if (!strncmp(messageRx, "\n", MAX_LEN)) {
            strncpy(messageRx, last_command, MAX_LEN);
        } else {
            strncpy(last_command, messageRx, MAX_LEN);
        }
        char messageTx[MAX_LEN];

        // Get the first token of the command
        char *tok = strtok(messageRx, " ");

        messageTx[0] = '\0'; 
        if(strncmp(messageRx, "help\n", strlen("help\n")) == 0) { 
            // This can be buggy as it determines the strlen of the help string
            sprintf(messageTx,  "Accepted command examples: \n"
                                "count       -- display total number of samples taken.\n"
                                "length      -- display number of samples in history.\n"
                                "history     -- display the full sample history being saved.\n"
                                "get 10      -- display the 10 most recent history values.\n"
                                "dips        -- display number of dips.\n"
                                "stop        -- cause the server program to end.\n");
        } else if(strncmp(messageRx, "count\n", strlen("count\n")) == 0) {
            sprintf(messageTx, "Number of samples taken = %lli.\n", Sampler_getNumSamplesTaken());

        } else if(strncmp(messageRx, "length\n", strlen("length\n")) == 0) {
            sprintf(messageTx, "History can hold  %i samples\nCurrently holding %i samples\n", Sampler_getHistorySize(), Sampler_getNumSamplesInHistory());
            //sprintf(messageTx, "Currently holding = %i samples\n", Sampler_getNumSamplesInHistory());

        } else if(strncmp(messageRx, "history\n", strlen("history\n")) == 0) {
            // get the current history of sample and return all the data
            //sprintf(messageTx, "Sample history: ");
            int length = 0;
            double *samples = Sampler_getHistory(&length);
            int historySize  = Sampler_getNumSamplesInHistory();//get copy of current sample in a new buffer
            // double currentLightSample[historySize];
            // circular_buf_returnBuf(buffer, currentLightSample);
            int samples_in_chunk = 0;
            for(int i =0; i< historySize; i++){
                samples_in_chunk++;                
                if (samples_in_chunk * SLOTS_PER_ENTRY >= MAX_LEN) {
                    Udp_transmitReply(sin_len, messageTx, (struct sockaddr *) &sinRemote);

                    // Reset the sample count for a chunk
                    samples_in_chunk = 0;

                    // Reset the chunk
                    strncpy(messageTx, "", MAX_LEN);
                        
                    // Since the current element cannot be fit into the chunk,
                    // decrement the value of i so that we can put it into the new
                    // chunk
                    i--;
                } else {
                        
                    if ((i+1) % 20 == 0)
                            // Adding a new line every 20 samples
                            snprintf(messageTx + strlen(messageTx), MAX_LEN-strlen(messageTx), "%.3f,\n",samples[i]);
                    else 
                            // Adding the sample into the chunk
                            snprintf(messageTx + strlen(messageTx), MAX_LEN-strlen(messageTx), "%.3f, ", samples[i]);
                }
            }
            free(samples);
            snprintf(messageTx + strlen(messageTx), MAX_LEN-strlen(messageTx), "\n");
           
            // // get current array that Sorter is sorting
            // double *new_arr = Sampler_getHistory(Sampler_getHistorySize());
            // char placeHolderBeforeWritingToMsgTx[MAX_LEN];
            // // clear content of placeHolder string
            // placeHolderBeforeWritingToMsgTx[0] = '\0';
            // int i = 0;
            // while(i < historySize) {
            //     char valAtIndexCommaSep[400]; 
            //     //int valAtIndex = new_arr[i];
            //     double valAtIndex = currentLightSample[i];
            //     if(i != 0 && i % 20 == 0 && i != historySize - 1) {
            //         // separate line prints at 10 values comma separated per line
            //         sprintf(valAtIndexCommaSep, "%0.3f,\n", valAtIndex);
            //     } else if(i != historySize - 1) {
            //         // regular in line comma separated values at index i
            //         sprintf(valAtIndexCommaSep, "%0.3f, ", valAtIndex); 
            //     } else {
            //         // last value of the array
            //         sprintf(valAtIndexCommaSep, "%0.3f\n", valAtIndex);
            //     }
            //     if((strlen(valAtIndexCommaSep) + strlen(placeHolderBeforeWritingToMsgTx)) >= MAX_LEN) {
            //         // the above boolean expression checks if 
            //         // the current comma separated string will overflow the placeHolder
            //         // which will overflow the message
            //         // if it can overflow, write to message and transmit reply right away
            //         sprintf(messageTx, "%s", placeHolderBeforeWritingToMsgTx);
            //         Udp_transmitReply(sizeof(sinRemote), messageTx, (struct sockaddr *) &sinRemote);
            //         // after transmitting, clear contents of both the reply message
            //         // and the placeHolder string to reduce risk of overflow
            //         memset(messageTx, '\0', sizeof(messageTx));
            //         memset(placeHolderBeforeWritingToMsgTx, '\0', sizeof(placeHolderBeforeWritingToMsgTx));
            //     } 
            //     // concatenate to placeHolder the "possibly overflow" value
            //     // or just every comma separated value
            //     strcat(placeHolderBeforeWritingToMsgTx, valAtIndexCommaSep);
            //     i++;
            // }
            // sprintf(messageTx, "%s", placeHolderBeforeWritingToMsgTx);
            // free(new_arr);
            
        } else if(strncmp(messageRx, "stop\n", strlen("stop\n")) == 0) {
            sprintf(messageTx, "Program Terminating\n");
            run = false;
            stopProgram = true;
        } else if(strncmp(messageRx, "get",MAX_LEN) == 0) { // return N most recent sample history values
            tok = strtok(NULL, " ");
            int n = atoi(tok);
            int valid_samples = Sampler_getNumSamplesInHistory();
            int length = 0;
            double *samples = Sampler_getHistory(&length);
            if (n <= valid_samples && n > 0) {
               
                int samples_in_chunk = 0;
                for (int i = valid_samples-n; i < valid_samples; i++) {    
                    samples_in_chunk++;                
                    if (samples_in_chunk * SLOTS_PER_ENTRY >= MAX_LEN) {
                        Udp_transmitReply(sin_len, messageTx, (struct sockaddr *) &sinRemote);

                        // Reset the sample count for a chunk
                        samples_in_chunk = 0;

                        // Reset the chunk
                        strncpy(messageTx, "", MAX_LEN);
                            
                        // Since the current element cannot be fit into the chunk,
                        // decrement the value of i so that we can put it into the new
                        // chunk
                        i--;
                    } else {
                        // Adding the sample into the chunk
                        snprintf(messageTx + strlen(messageTx), MAX_LEN-strlen(messageTx), "%.3f, ", samples[i]);
                    }

                    
                    snprintf(messageTx + strlen(messageTx), MAX_LEN-strlen(messageTx), "\n");
                }
            } else {
                snprintf(messageTx, MAX_LEN, "N is larger than the numbers of valid samples in history, please choose a number between 1 and %d\n", valid_samples);
            }
            free(samples);
            
            
            // 5 is chosen as the appropriate size,
            // less risk of overflow to a value at index
            // max of 2055
            // char indexValueChar[5]; 
            
            // for(int i = 4; i <= strlen(messageRx); i++) {
            //     indexValueChar[i - 4] = messageRx[i];
            // }
            // int indexValueInteger = atoi(indexValueChar);
            // indexValueChar[0] = '\0';
            // if(indexValueInteger <= 0 || indexValueInteger > Sorter_getArrayLength()) {
            //     sprintf(messageTx, "Invalid argument. Must be between 1 and %d (array length).\n", Sorter_getArrayLength());
            // } else {
            //     int *arrayData = Sorter_getArrayData(indexValueInteger);
            //     int retrievedValueAtIndex = arrayData[indexValueInteger - 1];
            //     sprintf(messageTx, "Value %d = %d\n", indexValueInteger, retrievedValueAtIndex);
            //     free(arrayData);
            // }
        } else {
            // check for invalid command
            sprintf(messageTx, "Invalid command, please refer to command 'help' for available commands.\n");
        }

        Udp_transmitReply(sizeof(sinRemote), messageTx, (struct sockaddr *) &sinRemote);
    }
    Udp_CloseConnection();
    return NULL;
}
    

