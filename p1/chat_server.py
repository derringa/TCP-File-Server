#
# Program: 		Synchronized chat server
# Author: 		Andrew Derringer
# Last Modified: 	2/5/2020
#
# Summary:		Params:	[1] The server user's username
# 				[3] The desired server port.
#
# 			Cond:	[1] Username no more than 10 characters.
# 				[2] Messages mo more than 500 characters.
# 				[3] Process will close on bad IP and port.
# 				[4] The * and \ are not to be used except
# 					for specific commands.
# 				[5] Typing '\quit' will end processes for
# 					both server and client.
#
# 			This server establishes a foreground listening
#			socket which waits for a connection from a client.
#			Their communication is synchronous one after another.
#			The * is reserver as an end of transmission character
#			and the program can be quit by entering '\quit'.


import socket
import threading
import sys
import os
import random
import _thread
import time


class ThreadServer(object):
    '''
    Summary:	Socket server process binds on init and begins
		lisitening on listen() call. End upon user input
		'\quit'. Should not send or receive * character.
    '''
    def __init__(self, username, host, port):
        self.username = username
        self.host = host
        self.port = port

        # Define socket and bind to entered port.
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((self.host, self.port))

    def listen(self):
        '''
        Summary:	Calling method to activate the server.
        Outcomes:	[1] Accept client connections.
        '''
        self.sock.listen(5)
        
        while True:
            client, address = self.sock.accept()
            self.listenToClient(client,address)


    def listenToClient(self, client, address):
        '''
        Summary:	Receives input from accepted connection
			and receives user input to send message
			or quit process.
	Param:		[1] Client connection FD.
			[2] Client information.
	Outcomes:	[1] Prints messages received from client.
			[2] Accepts user input to send to client.
			[3] Closes process upon user command.
        '''
        max_size = 510
        total = ''

        # Loop receives packets of client data until end reached.

        while True:
            try:
                data = client.recv(max_size)
                if data: 
                    string = data.decode('utf-8')
                    total += string
                    if "*" in string:
                       break
                else:
                   break
            except:
                break

        # If client sent terminating character then print
        # message and close application else just print.

        if '\\' in total:
           print(total[:-3])
           client.close()
           sys.exit()
        else:
           print(total[:-1])

	# Loop user input until it meets length.

        response = str(input(''))
        while len(response) > max_size:
           print("ERROR: Message exceeded max characters (500).")
           response = str(input(''))

	# If user chooses quit the send termination message to client and quit
	# else send message and close client connection but keep listening.

        if response == "\\quit":
           response = "<NOTICE> The other user has left. Your program is closing.\\*"
           client.send(response.encode())
           client.close()
           sys.exit()
        else:
           response = "<" + self.username + ">" + response + "*"
           client.send(response.encode())
           client.close()
           

if __name__ == "__main__":
    if len(sys.argv[1]) > 10:
        print("ERROR: Username exceeded max characters (10).")
    else:
        ThreadServer(sys.argv[1], '',int(sys.argv[2])).listen()
