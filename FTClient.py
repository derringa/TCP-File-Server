'''
Program:	TCP Socket File Server
Author:		Andrew Derringer
Last Modified:	3/6/2020
Summary:	File server client program. It can retreive server working directory contents, copy text files,
		and change work directories.
Sys Args:	[format] hostname port command (file/directory name)
		Available commands include:
		[1] "-l" returns a list of files in server working directory.
		[2] "g filename" copies valid file from server working directory into client working directory.
		[3] "-d" returns a list of subdirectories and parent directory from server.
		[4] "cd directory" changes the server working directory.
'''


import os
import sys
import socket
import random


class FTClient(object):
     '''
     Summary:	Establishes a TCP client socket and attempts to send information
		passed at initialization via command line arguments.
     Param:	[1] host - string.
     		[2] port - string.
		[3] command - string listed in header.
		[4] server_ent - string required for copying file or changing directory.
     '''
     random.seed(1) # Temp file name generation.

     def __init__(self, host, port, command, server_ent=None):
          self.command = command
          self.server_ent = server_ent
          self.delimiter = "<J#J1J3>"
 
          self.sock = self.socketConnect(host, port)
          self.socketSend()
          self.socketRecv()


     def socketConnect(self, host, port):
          '''
          Summary:	Create socket object and connect to server host:port.
          Param:	[1] host - string.
 			[2] port - string.
          Ret:		[1] socket object.
          '''

          # Create socket(). TCP Protocol. IPv4 or IPv6.
          try:
               sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
          except socket.error as e:
               print("Client Error: Create socket - \"{}\"".format(e))
               sys.exit(1)

          # Socket connect().
          try:
               sock.connect((host, int(port)))
          except socket.error as e:
               print("Client Error: Socket connect \"{}\"".format(e))
               sys.exit(1)

          print("Client: Connected to server on {}:{}.".format(host, port))
          return sock


     def socketSend(self):
          '''
          Summary:	Socket send commands passed by user at startup via command-line.
			Object self.command sent every call. Conditionally self.server_ent.
          '''

          # Message build varies by command type.
          if self.server_ent == None:
               message = self.command + self.delimiter
          else:
               message = self.command + self.server_ent + self.delimiter

          send_length = len(message)
          total_sent = 0

          # Socket send().
          while total_sent < send_length:
               sent = self.sock.send(message[total_sent:].encode())
               if sent == 0:
                    raise RuntimeError("Client Error: socket send")
               total_sent += sent


     def socketRecv(self):
          '''
          Summary: 	When called socket waits on recv until unique delimiter is reached.
			Contents handed off to manageRecv for processing.
          '''

          # Create file either to match file being copied or temp for other returns.
          if self.command == "-g" and self.server_ent != None:
               fname = self.server_ent
          else:
               fname = "temp" + str(random.randint(10000,99999)) + ".txt"

          # Socket recv() until delimiter reached for error.
          try:
               with open(fname, 'w') as fd:
                    while True:
                         try:
                              buff = self.sock.recv(1000).decode('utf-8')
                         except socket.error as e:
                              print("Client Error: socket recv.")
                              fd.close()
                              sys.exit(1)

                         if not buff:
                              break
                         elif self.delimiter in buff:
                              fd.write(buff[:-8])
                              break
                         else:
                              fd.write(buff)

               fd.close()
          except OSError:
               print("Client Error: file write.")
               sys.exit(1)

          # Process data received.
          self.manageRecv(fname)


     def manageRecv(self, fname):
          '''
          Summary:	Unless successful file copy from server, read file contents
			of server message from temp file and delete file.
          Param:	[1] fname - string
          '''

          # Check for potential error header in server response.
          try:
               fd = open(fname, 'r')
               error_check = fd.read(12)
               fd.close()
          except OSError:
               print("Client Error: File open.")

          # In all cases except successful file copy read response and delete temp file.
          if self.command != "-g" or error_check == "Server Error":
               with open(fname, 'r') as fd:
                    print(fd.read(), end="")
               fd.close()
          
               try:
                    os.remove(fname)
               except OSError:
                    print("Client Error: file delete.")


def verifyCommand(command):
     '''
     Summary:	Validate command-line entries against list of commands options.
     '''
     valid_commands = ["-l", "-g", "-d", "cd"]
     if command not in valid_commands:
          print("Client Error: Invalid command")
          sys.exit(1)


if __name__ == "__main__":
     if len(sys.argv) == 4:
          verifyCommand(sys.argv[3])
          FTClient(sys.argv[1], sys.argv[2], sys.argv[3])
     elif len(sys.argv) == 5 and (sys.argv[3] == "-g" or sys.argv[3] == "cd"):
          verifyCommand(sys.argv[3])
          FTClient(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
     else:
          print("Client Error: usage runClient host port command file(if command==-g)")
          sys.exit(1)
