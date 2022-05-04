server_unix:
	gcc server.c socket_utils.c -o server -luuid
ordinary_client:
	gcc client.c socket_utils.c -o client -luuid
admin_client:
	gcc admin.c socket_utils.c -o admin
