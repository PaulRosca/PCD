server_unix:
	gcc server.c socket_utils.c soapC.c soapServer.c -o server -luuid -lgsoap
ordinary_client:
	gcc client.c socket_utils.c -o client -luuid
admin_client:
	gcc admin.c socket_utils.c -o admin
