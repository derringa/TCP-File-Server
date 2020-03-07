/**********************************************************
 * Summary:		File server program. Returns files or subdiractorys in current directory,
 * 			copies files to destination, and changes directories on command.
 * Author:		Andrew Derringer
 * Last Modified:	3/6/2020
**********************************************************/


#ifndef FTSERVER_HPP
#define FTSERVER_HPP


#include <string>
#include <vector>
#include <netinet/in.h>


using std::string;
using std::vector;
using std::fstream;


class FTServer {
     private:
          string delimiter;
          string port;
          int max_threads;
          int active_threads;
          int socket_FD;
          string mycwd;

     public:
          FTServer(string);
          ~FTServer();

          // Establish mycwd string and change on command.
          string string_cwd();
          string change_cwd(string);

          // Socket(), Bind(), Listen(), and thread on Accept().
          void bindSocket();
          void listenOnPort();

          // Thread call Recv().
          void manageThread(struct sockaddr_storage, int);
          void *get_in_addr(struct sockaddr *);

          // Process client message from Recv().
          void manageReq(int, string);
          vector<string> listDir(string);
          void sendData(int, string);
};


#endif
