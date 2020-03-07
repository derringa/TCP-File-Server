/**********************************************************
 * Summary:		File server program. Returns files or subdiractorys in current directory,
 * 			copies files to destination, and changes directories on command.
 * Sys Args:		Format: runServer port
 * 			[1] port - string
 * Outputs:		By client commands:
 * 			[1] "-l" returns a list of files in server working directory.
 *			[2] "g filename" copies file from server directory into client directory.
 *			[3] "-d" returns a list of subdirectories and parent directory from server.
 *			[4] "cd directory" changes the server working directory.
 * Author:		Andrew Derringer
 * Last Modified:	3/6/2020
**********************************************************/


#include <thread>
#include <iostream>


#include <algorithm>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <time.h>
#include <stdlib.h>


#include "FTServer.hpp"


#define RECV_BUF 100
#define SEND_BUF 1000
#define CWD_BUF 1000

using std::cout;
using std::endl;
using std::cerr;


FTServer::FTServer(string port_arg) {

     srand(time(0));

     delimiter = "<J#J1J3>"; 
     mycwd = string_cwd();
     port = port_arg;
     max_threads = 5;
     active_threads = 0;
     bindSocket(); //assigns socket_FD
     listenOnPort();

}


FTServer::~FTServer() {
     close(socket_FD);
}


string FTServer::string_cwd() {
     char cwd[CWD_BUF];
     getcwd(cwd, sizeof(cwd));
     string cwd_str(cwd);
     return cwd_str;
}

/*
 * Summary:	Modifies string corresponding to working dicectory by client command.
 * Param:	[1] dir - directory name entered by client.
*/
string FTServer::change_cwd(string dir) {

     // If .. entered remove last directory name from string else append entered.
     if (dir == "..") {
          int idx = mycwd.find_last_of("/\\");
          mycwd = mycwd.substr(0, idx);
     }
     else {
          mycwd += "/" + dir;
     }
}

/*
 *Summary: Socket setup. Socket(), Bind(), Listen().
*/
void FTServer::bindSocket() {

     int status;
     //struct sockaddr_storage server_addr;
     struct addrinfo serverspecs, *serverinfo;

     // Setup server socket information.
     memset(&serverspecs, '\0', sizeof(serverspecs));
     serverspecs.ai_family = AF_UNSPEC; //IPv4 or IPv6.
     serverspecs.ai_socktype = SOCK_STREAM; //TCP stream sockets.
     serverspecs.ai_flags = AI_PASSIVE; //Auto fill-in my IP.

     if ( (status = getaddrinfo(NULL, port.c_str(), &serverspecs, &serverinfo)) == -1 ) {
          cerr << "Server Error: getaddrinfo - " << gai_strerror(status) << endl; exit(1);
     }

     // Socket().
     if ( (socket_FD = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol) ) == -1) {
          cerr << "Server Error: socket." << endl; exit(1);
     }

     // Bind().
     if ( bind(socket_FD, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1 ) {
          cerr << "Server Error: bind." << endl; exit(1);
     }

     // Listen().
     if ( listen(socket_FD, max_threads) == -1 ) {
          cerr << "Server Error: listen." << endl; exit(1);
     }

     cout << "Server open on " << port << endl;

     freeaddrinfo(serverinfo);
}


/*
 * Summary: 	While loop waits on socket Accept() then creates thread process that
 *		recv() from connection. Maintains that max declared as member variable.
*/
void FTServer::listenOnPort() {
     struct sockaddr_storage client_addr;
     socklen_t client_size;
     int connection_FD;

     // Socket listen loop.
     while (1) {
          client_size = sizeof(client_addr);

          // Socket Accept().
          if ( (connection_FD = accept(socket_FD, (struct sockaddr *)&client_addr, &client_size)) == -1 ) {
               cerr << "Server Error: accept." << endl;
          }
          // Do not surpass max threads.
          else if (active_threads >= max_threads) {
               cerr << "Server Error: max threads." << endl;
          }
          // Thread(). Send connection FD and connecting client address info.
          else {
               std::thread thread_obj(&FTServer::manageThread, this, client_addr, connection_FD);
               thread_obj.detach();
          }
    }
}


/*
 * Summary:	Receive message from connecting client, format, and send on
 * 		to be processed.
 * Param:	[1] client_addr - information of sending client.
 * 		[2] connection_FD - File description for connection to be processed.		
*/
void FTServer::manageThread(struct sockaddr_storage client_addr, int connection_FD) {

     char s[INET6_ADDRSTRLEN]; 
     string  recv_total;
     char recv_segment[RECV_BUF];
     int recv_size;
     bool receiving = true;

     active_threads++;

     // Unpack client address inf to get IP.
     inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof(s));
     cout << "Server: Connection from " << s << endl;
   
     // Socket Recv() loop until delimiter reached.
     while(receiving) {
          memset(recv_segment, '\0', sizeof(recv_segment));
          if ( (recv_size = recv(connection_FD, recv_segment, sizeof(recv_segment), 0)) == -1 ) {
               cerr << "Server Error: recv." << endl; close(connection_FD); active_threads--; exit(1);
          }
          else {
               recv_total.append(recv_segment, recv_size);
               if (strstr(recv_segment, "<J#J1J3>") != NULL) {
                    receiving = false;
                    recv_total.erase(recv_total.length() - 8);
              }
          }
     }

     // Pass received message to be processed.
     manageReq(connection_FD, recv_total);
     close(connection_FD);  active_threads--; 
}


/*
 * Summary: 	Unpack sockaddr object depending on IPv4 or IPv6.
 * Param:	[1] sa - Connecting client information.
 * Citation:	Healper function from Beej's Guide to Network Programming.
*/
void *FTServer::get_in_addr(struct sockaddr *sa) {

     // If IPv4 else IPv6.
     if (sa->sa_family == AF_INET) {
          return &(((struct sockaddr_in*)sa)->sin_addr);
     }
     else {
          return &(((struct sockaddr_in6*)sa)->sin6_addr);
     }
}


/*
 * Summary:	Branching action taken based on message passed from connecting client recv().
 * 		All responses either from existing file or from temp file made until send complete.
 * Param:	[1] connection_FD - Open file descriptor for current client connection.
 * 		[2] recv_total - Client message processed for function.
 * Outcomes:	[1] Client requests file by name.
 * 		[2] Client changes server working directory.
 * 		[3] Client requests list of files in directory.
 * 		[4] Client requests list of associated directories.
 * 		[5] Client request error.
*/
void FTServer::manageReq(int connection_FD, string recv_total) {

     vector<string> dir_files = listDir("files");
     vector<string> dir_directories = listDir("directories");
     string command = recv_total.substr(0,2);
     string tempfile = mycwd + "/temp";
     FILE *temp_FD;
     string out_string;

     // Request to copy file with valid file name.
     if (command == "-g" && (std::find(dir_files.begin(), dir_files.end(), recv_total.substr(2)) != dir_files.end())) {
          sendData(connection_FD, mycwd + "/" + recv_total.substr(2));
     }
     // Request to change directory with valid directory name.
     else if ( command == "cd" && (std::find(dir_directories.begin(), dir_directories.end(), recv_total.substr(2)) != dir_directories.end())) {
          change_cwd(recv_total.substr(2));
     }
     // Requests that did not require additional file/directory argument.
     else {
          // Create temp file to hold message being sent to client.
          tempfile.append(std::to_string(10000 + (std::rand() % (99999 - 10000 + 1))));
          if ( (temp_FD = fopen(tempfile.c_str(), "w")) == NULL ) {
               cerr << "Server Error: openning temp file to write." << endl;
          }

          // Request for list of files in working directory.
          if (command == "-l") {
               for(vector<string>::const_iterator i = dir_files.begin(); i != dir_files.end(); i++) {
                    out_string = *i;
                    fprintf(temp_FD, "%s\n", out_string.c_str());
               }
               //fseek(temp_FD, -1, SEEK_END);
          }
          // Request for list of neighboring directories.
          else if (command == "-d") {
               for(vector<string>::const_iterator i = dir_directories.begin(); i != dir_directories.end(); i++) {
                    out_string = *i;
                    if (out_string != ".") {
                         fprintf(temp_FD, "%s\n", out_string.c_str());
                    }
               }
               //fseek(temp_FD, -1, SEEK_END);
          }
          // Invalid Request.
          else {
               fprintf(temp_FD, "%s\n", "Server Error: Invalid file or directory request.");
          }
          fclose(temp_FD);
          sendData(connection_FD, tempfile);

          // Delete temp file.
          if (remove(tempfile.c_str()) != 0) {
               cerr << "Server Error: Cannot delete temp file." << endl;
          }
     }

}

/*
 * Summary:	Receives name of file in current working directory. Opens and socket
 * 		send() content to client.
 * Param:	[1] connection_FD - Open file descriptor for current client connection.
 * 		[2] filename - Existing file if transfer else temp file containing message.
*/
void FTServer::sendData(int connection_FD, string filename) {

     // File open().
     FILE *fd;
     if ( (fd = fopen(filename.c_str(), "r")) == NULL ) {
          cerr << "Server Error: openning send file to read." << endl;
     }

    
     char send_segment[SEND_BUF];
     memset(send_segment, '\0', sizeof(send_segment));

     // Socket Send().
     while(fgets(send_segment, sizeof(send_segment), fd) != NULL) {
         if (send(connection_FD, send_segment, strlen(send_segment), 0) == -1) {
                cerr << "Server Error: send." << endl; close(connection_FD); active_threads--; exit(1);
          }
          memset(send_segment, '\0', sizeof(send_segment)); 
     }

     // Send message ending delimiter last.
     if (send(connection_FD, delimiter.c_str(), strlen(delimiter.c_str()), 0) == -1) {
          cerr << "Server Error: send." << endl; close(connection_FD); active_threads--; exit(1);
     }

     fclose(fd);

}

/*
 * Summary:	Create and return string vector of either files in directory or
 * 		associated directories by passed request.
 * Param:	[1] content_type - Either "files" or "directories".
*/
vector<string> FTServer::listDir(string content_type) {

     vector<string> dir_files;
     vector<string> dir_directories;

     struct dirent *de;
     DIR *dr = opendir(mycwd.c_str());
     if (dr == NULL) {
          cerr << "Server Error: Cannot open current directory." << endl; exit(1);
     }

     // Loop iterator reads directory entities until NULL. Add to either file or directory list.
     while ( (de = readdir(dr)) != NULL ) {
          if (de->d_type == DT_REG) {
               dir_files.push_back(de->d_name);
          }
          else {
               dir_directories.push_back(de->d_name);
          }
     }

     // Return depends on parameter request.
     if (content_type == "files") {
          return dir_files;
     }
     else if (content_type == "directories") {
          return dir_directories;
     }
}
