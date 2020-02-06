/********************************************************************
 * Program: 		Synchronized chat client
 * Author: 		Andrew Derringer
 * Last Modified: 	2/5/2020
 *
 * Summary:		Params:	[1] The client user's username.
 * 				[2] The desired user IP address.
 * 				[3] The desired server port.
 *
 * 			Cond:	[1] Username no more than 10 characters.
 * 				[2] Messages mo more than 500 characters.
 * 				[3] Process will close on bad IP and port.
 * 				[4] The * and \ are not to be used except
 * 					for specific commands.
 * 				[5] Typing '\quit' will end processes for
 * 					both server and client.
 *
 * 			This client establishes a socket connection
 * 			with an already bound and listening server
 * 			process. Messages must be send synchronously
 * 			one after the other. The * symbol is used as
 * 			an end of transmission confirmation and should
 * 			not be used on the command line. A \ is reserved
 * 			exclusively for the special command '\quit'.
********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>

// Specified username and message maximums.
#define NAME_SIZE 10
#define MSG_SIZE 500


// Error function used for reporting issues
void error(const char *msg) { perror(msg); exit(0); }


int main(int argc, char *argv[]) {

   // Socket related variables.
   int socketFD, portNumber, charsWritten, charsRead;
   struct sockaddr_in serverAddress;
   struct hostent* serverHostInfo;

   // Input and output related buffers.
   char user_input[MSG_SIZE];
   char input_chunk[MSG_SIZE];
   char input_buffer[MSG_SIZE];
   char output_buffer[NAME_SIZE + MSG_SIZE];


   if (argc < 4) {
      fprintf(stderr,"USAGE: %s username host port\n", argv[0]);
      exit(1);
   }


   char username[NAME_SIZE + 1];
   memset(username, '\0', sizeof(username));
   strcat(username, argv[1]);
   if (username[sizeof(username) - 1] != '\0') {
      error("ERROR: Username exceeded max characters (10).\n");
   }


   /***************************************
   * Set up the server address struct
   ***************************************/

   memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
   portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
   serverAddress.sin_family = AF_INET; // Create a network-capable socket
   serverAddress.sin_port = htons(portNumber); // Store the port number
   //serverAddress.sin_addr.s_addr = inet_addr("128.193.54.182");
   serverHostInfo = gethostbyname(argv[2]); // Store the host
   if (serverHostInfo == NULL) {
      fprintf(stderr, "ERROR: Failed to apply server information.\n");
      exit(1);
   }
   memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address


   char* sp;
   memset(user_input, '\0', sizeof(user_input));


   /**************************************
   * Send-Recv Loop:
   * While user hasn't entered quit command
   * continue loop of send and recv.
   **************************************/


   while (strcmp(user_input, "\\quit\n") != 0) {
      memset(input_buffer, '\0', sizeof(input_buffer));
      memset(output_buffer, '\0', sizeof(output_buffer));
      memset(user_input, '\0', sizeof(user_input));

      // Generate socket FD and connect to sertver.

      socketFD = socket(AF_INET, SOCK_STREAM, 0);
      if (socketFD < 0) {
         error("ERROR: Failed to create socket.");
      }
      if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
         error("ERROR: Failed to connect socket.");
      }

      // User input and socket send.

      if (fgets(user_input, sizeof(user_input), stdin)) {

        // Check for buffer overflow. Message too long. 

        if (user_input[sizeof(user_input) - 1] == '\0') {

            // If user entered quit command send message to signal end to server.
            // else send <username>message.

            if (strcmp(user_input, "\\quit\n") != 0) {
               sp = strchr(user_input, '\n');
               *sp = '\0';
               sprintf(output_buffer, "<%s>%s*", username, user_input);
            }
            else {
               sprintf(output_buffer, "<%s>%s*", "NOTICE", "The Other user has left. Your process is closing.\\*"); 
            }

            // Send message and error check.

            charsWritten = send(socketFD, output_buffer, strlen(output_buffer), 0);
            if (charsWritten < 0) {
               error("Error: failed writing to socket");
            }
            if (charsWritten < strlen(output_buffer)) {
               error("ERROR: Not all data written to socket.\n");
            }
       
         }
         else {
            error("ERROR: Message exceeded max characters (500).\n");
         }
      
         // Socket recv and print.
         // Run loop to catch entire string if user hasn'y already chosen to quit.

         while(strcmp(user_input, "\\quit\n") != 0) {
            memset(input_chunk, '\0', sizeof(input_chunk));
            charsRead = recv(socketFD, input_chunk, sizeof(input_chunk) - 1, 0); // Read data from the socket.

            // If * received then end of message reached with that packet. Break.
            // Else append to previously received material and wait for another recv.
           
            if (charsRead < 0) {
               error("CLIENT: ERROR reading from socket");
            } 
            else if ((sp = strchr(input_chunk, '*')) != NULL) {
               *sp = '\0'; // Remove * from printed message.
               strcat(input_buffer, input_chunk);
               break;
            } 
            else { // Recv is good but not done.
               strcat(input_buffer, input_chunk);
            }
         }

         // If termination received from other process then change input_buffer to quit command. End loop.

         if ((sp = strchr(input_buffer, '\\')) != NULL) {
            memset(user_input, '\0', sizeof(user_input));
            strcat(user_input, "\\quit\n");
            *sp = '\0';
         }

         // Print recv and close connection.

         printf("%s\n", input_buffer);
         fflush(stdin);
         close(socketFD);

      }
   }

   return 0;

}
