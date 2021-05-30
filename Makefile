comp_server:
	gcc -o server.out rtmp_server.c streamsegmenter.c
segment:
	gcc -o seg.out streamsegmenter.c
