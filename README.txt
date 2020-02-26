To compile:
	gcc chat_server -lpthread -o server
	gcc chat_client -o client
	
To run:
	./server <PORT> 			(THIS ONLY NEEDS TO BE RUN 1 TIME)
	./client <IP of server> <PORT> <euid>	(THIS CAN BE RUN UP TO 30 TIMES AT ONCE)
	
COMMANDS:
	-SEND <*> OR <username>
		- Sends a message to everyone on the server with "*", or to a specific user
	-LIST
		- Lists the other users currently on the server
	-QUIT
		- Client will quit the server and server will update accordingly
	-BLOCK <username>
		- Blocks from seeing a user's messages
	-UNBLOCK <username>
		- Unblocks a user
	-HELP
		- Lists the available commands
