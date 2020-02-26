To compile:
	gcc chat_server -lpthread -o server
	gcc chat_client -o client
	
To run:
	./server <PORT> 						(THIS ONLY NEEDS TO BE RUN 1 TIME)
	./client <IP of server> <PORT> <euid>	(THIS CAN BE RUN UP TO 30 TIMES AT ONCE)