server_unix:
	gcc server.c -o server -luuid
ordinary_client:
	gcc client.c -o client -luuid
