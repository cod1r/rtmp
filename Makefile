comp_server:
	gcc -o rtmp_server rtmp_server.c streamsegmenter.c
exec_server:
	./rtmp_server 
comp_client:
	gcc -o rtmp_client rtmp_client.c -Wall -Wextra
exec_client:
	./rtmp_client
both_server:
	gcc -o rtmp_server rtmp_server.c
	./rtmp_server
both_client:
	gcc -o rtmp_client rtmp_client.c
	./rtmp_client
segment:
	gcc -o seg streamsegmenter.c
