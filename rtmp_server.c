#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "rtmp_types.h"
#include "streamsegmenter.h"

// NETWORK BYTE ORDER IS BIG ENDIAN; MY SYSTEM IS LITTLE ENDIAN 
// the backlog argument defines the max length to which the queue of pending connections for sockfd may grow.
// BASIC HEADER, MESSAGE HEADER, EXTENDED TIMESTAMP, CHUNK DATA
// SPEC SAYS MSG HEADER 0 MUST BE USED AT THE BEGINNING OF THE CHUNK STREAAMMMM!!!

// reads the first byte sent; 
// and then depending on the last 6 bits, we either read in basic header 2 or 3
// then depending on the format or first 2 bits:
// 0 -> type 0 message header, we read in 11 bytes of data
// 1 -> type 1 message header, we read in 7 bytes of data
// 2 -> type 2 message header, we read in 3 bytes of data
// 3 -> we use previous parameters
// which means the chunk HEADER size itself is the message header plus basic header

// chunk size is payload plus header size
// chunk size is different from message length; message length is the size of the message that could include multiple chunks
// default chunk size is 128 bytes
#define BACK_LOG 50

void prepare_time(unsigned char timestamp[]) {
	unsigned int t = get_current_time();
	timestamp[2] = t;
	timestamp[1] = t >> 8;
	timestamp[0] = t >> 16;
}
// for big endian
unsigned int get_size(unsigned char msg_length[]) {
	return msg_length[0]*(1 << 16) + msg_length[1]*(1 << 8) + msg_length[2];
}

void get_msg_header(int sfd, int type, void* container) {
	switch (type) {
		case 0: {
			chunk_msg_header_0_t* c = (chunk_msg_header_0_t*)container;
			ssize_t _1 = recv(sfd, &(c->timestamp), sizeof(c->timestamp), 0);
			ssize_t _2 = recv(sfd, &(c->msg_length), sizeof(c->msg_length), 0);
			ssize_t _3 = recv(sfd, &(c->msg_typeid), sizeof(c->msg_typeid), 0);
			ssize_t _4 = recv(sfd, &(c->msg_streamid), sizeof(c->msg_streamid), 0);
			break;
		}
		case 1: {
			chunk_msg_header_1_t* c = (chunk_msg_header_1_t*)container;
			ssize_t _1 = recv(sfd, &(c->timestamp_delta), sizeof(c->timestamp_delta), 0);
			ssize_t _2 = recv(sfd, &(c->msg_length), sizeof(c->msg_length), 0);
			ssize_t _3 = recv(sfd, &(c->msg_typeid), sizeof(c->msg_typeid), 0);
			break;
		}
		case 2: {
			chunk_msg_header_2_t* c = (chunk_msg_header_2_t*)container;
			ssize_t _1 = recv(sfd, &(c->timestamp_delta), sizeof(c->timestamp_delta), 0);
			break;
		}
		default: {
			printf("header type not implemented\n");
			exit(EXIT_FAILURE);
			break;
		}
	}
}

// for message header type 0
void send_chunk(int sfd, unsigned char cs_id, unsigned int msg_length, unsigned char msg_typeid, unsigned int msg_streamid) {
	chunk_basic_header_1_t first;
	first.first_byte = cs_id;
	chunk_msg_header_0_t msg;
	prepare_time(msg.timestamp);
	// needs to be in big endian order
	// this is how you would put a number into your structs in big endian form
	msg.msg_length[2] = msg_length;
	msg.msg_length[1] = msg_length >> 8;
	msg.msg_length[0] = msg_length >> 16;
	msg.msg_typeid = msg_typeid;
	msg.msg_streamid = htonl(msg_streamid);
	// basic header
	ssize_t _1 = send(sfd, &(first.first_byte), sizeof(first.first_byte), 0);
	// msg header
	ssize_t _2 = send(sfd, &(msg.timestamp), sizeof(msg.timestamp), 0);
	ssize_t _3 = send(sfd, &(msg.msg_length), sizeof(msg.msg_length), 0);
	ssize_t _4 = send(sfd, &(msg.msg_typeid), sizeof(msg.msg_typeid), 0);
	ssize_t _5 = send(sfd, &(msg.msg_streamid), sizeof(msg.msg_streamid), 0);
	// we send payload after this function
}
/*
	Types of messages:
		Protocol control messages
			Has to be sent on chunk stream 2 and message stream id 0. Message type id is 4
		User Control Event Messages
			Has to be sent on chunk stream 2 and message stream id 0. Message type id is 4

		Protocol messages and User control event messages use message types 1, 2, 3, 4, 5, 6

		Audio/Video messages
			Has message type id 8 for audio and 9 for video

		Command Messages
			has message type id 20, for amf0 encoding
			has message type id 17, for amf3 encoding
			Usually has AMF objects

			Types of (NetConnection)commands:
				connect
				call
				close
				createStream

			Types of (NetStream)commands:
				play
				play2
				deleteStream
				closeStream
				receiveAudio
				receiveVideo
				publish
				seek
				pause

		Messages that we might not use:
			Data messages
			Shared Object Messages

*/
void send_streambegin(int sfd, unsigned short event_type, unsigned int streamid) {
	unsigned short etype = htons(event_type);
	send(sfd, &etype, 2, 0);
	unsigned int sid = htonl(streamid);
	send(sfd, &sid, 4, 0);
}

void send_window_acknowledgement(int sfd, int size) {
	unsigned int was = htonl(size);
	send(sfd, &was, sizeof(was), 0);
}

void send_set_peer_bandwidth(int sfd, unsigned char limit_type, unsigned int window_acknowledgement_size) {
	send_window_acknowledgement(sfd, window_acknowledgement_size);
	send(sfd, &limit_type, sizeof(limit_type), 0);
}

void onConnect(int sfd, unsigned char tid[8]) {
	unsigned char stringcoremarker = 2;
	unsigned short length = htons(7);
	send(sfd, &stringcoremarker, 1, 0);
	send(sfd, &length, 2, 0);
	send(sfd, "_result", strlen("_result"), 0);
	unsigned char numbercoremarker = 0;
	send(sfd, &numbercoremarker, 1, 0);
	send(sfd, tid, 8, 0);
	unsigned char nullmarker = 5;
	send(sfd, &nullmarker, 1, 0);
}

void onCreateStream(int sfd, unsigned char tid[8]) {
	unsigned char stringcoremarker = 2;
	unsigned short length = htons(7);
	send(sfd, &stringcoremarker, 1, 0);
	send(sfd, &length, 2, 0);
	send(sfd, "_result", strlen("_result"), 0);
	unsigned char numbercoremarker = 0;
	send(sfd, &numbercoremarker, 1, 0);
	send(sfd, tid, 8, 0);
	unsigned char nullmarker = 5;
	send(sfd, &nullmarker, 1, 0);
}

void onPublish(int sfd, unsigned char tid[8]) {
	unsigned char stringcoremarker = 2;
	unsigned short cmd_len = htons(strlen("onStatus"));
	unsigned char numbercoremarker = 0;

	send(sfd, &stringcoremarker, 1, 0);
	send(sfd, &cmd_len, 2, 0);
	send(sfd, "onStatus", strlen("onStatus"), 0);
	send(sfd, &numbercoremarker, 1, 0);
	send(sfd, tid, 8, 0);
	unsigned char nullcoremarker = 5;
	send(sfd, &nullcoremarker, 1, 0);

	unsigned char objectcoremarker = 3;
	send(sfd, &objectcoremarker, 1, 0);

	unsigned short level_len = htons(5);
	send(sfd, &level_len, 2, 0);
	char* level = "level";
	send(sfd, "level", strlen("level"), 0);

	send(sfd, &stringcoremarker, 1, 0);
	unsigned short status_len = htons(6);
	send(sfd, &status_len, 2, 0);
	char* status = "status";
	send(sfd, "status", strlen("status"), 0);

	unsigned short code_len = htons(4);
	send(sfd, &code_len, 2, 0);
	send(sfd, "code", strlen("code"), 0);

	send(sfd, &stringcoremarker, 1, 0);
	unsigned short codemsg_len = htons(23);
	send(sfd, &codemsg_len, 2, 0);
	send(sfd, "NetStream.Publish.Start", strlen("NetStream.Publish.Start"), 0);

	unsigned short desc_len = htons(11);
	send(sfd, &desc_len, 2, 0);
	send(sfd, "description", strlen("description"), 0);

	send(sfd, &stringcoremarker, 1, 0);
	unsigned short descmsg_len = htons(3);
	send(sfd, &descmsg_len, 2, 0);
	send(sfd, "lol", strlen("lol"), 0);

	unsigned short two_zeroes = htons(0);
	unsigned char objend = 9;
	send(sfd, &two_zeroes, 2, 0);
	send(sfd, &objend, 1, 0);
}
void parse_command_msg(int sfd, unsigned int cs_id, unsigned char payload[], int size, unsigned int window_acknowledgement_size) {
	// looping through the bytes that we get
	// these are amf encoded
	// first byte is always the string core marker
	// next two bytes is how long the coming string is
	unsigned short length = payload[1]*(1 << 8) + payload[2];
	char* command_msg = malloc(length);
	// gets the command msg in the payload
	int INDEX = 3;
	for (int i = 0; i < length; ++i) {
		command_msg[i] = payload[i+3];
		if (i + 1 == length) {
			command_msg[i+1] = '\0';
			INDEX = i + 2 + 3;
		}
	}
	printf("command msg: %s\n", command_msg);	
	unsigned char transactionid[8];
	int temp_index = INDEX;
	for (int j = temp_index; j < temp_index+8; ++j) {
		transactionid[j-temp_index] = payload[INDEX++];
	}
	
	// TODO: we need to parse the objects that we get in the message; if there are any
	// -------------------------------------------------------------------------------
	if (strncmp(command_msg, "connect", strlen(command_msg)) == 0) {
		send_chunk(sfd, cs_id, 4, 5, 0);
		send_window_acknowledgement(sfd, window_acknowledgement_size);

		send_chunk(sfd, cs_id, 5, 6, 0);
		send_set_peer_bandwidth(sfd, 0, window_acknowledgement_size);

		send_chunk(sfd, cs_id, 6, 4, 0);
		send_streambegin(sfd, 0, cs_id);

		send_chunk(sfd, cs_id, 20, 20, 0);
		onConnect(sfd, transactionid);
		printf("responded to connect msg...\n");
	}
	else if (strncmp(command_msg, "call", strlen(command_msg)) == 0) {
		printf("call command msg...\n");
	}
	else if (strncmp(command_msg, "close", strlen(command_msg)) == 0) {
		printf("closing...\n");
	}
	else if (strncmp(command_msg, "createStream", strlen(command_msg)) == 0) {
		send_chunk(sfd, cs_id, 20, 20, 0);
		onCreateStream(sfd, transactionid);
		printf("responded to createStream msg...\n");
	}
	else if (strncmp(command_msg, "play", strlen(command_msg)) == 0) {
	}
	else if (strncmp(command_msg, "play2", strlen(command_msg)) == 0) {
	}
	else if (strncmp(command_msg, "deleteStream", strlen(command_msg)) == 0) {
		printf("deleteStream command msg...\n");
	}
	else if (strncmp(command_msg, "closeStream", strlen(command_msg)) == 0) {
		printf("closeStream command msg...\n");
	}
	else if (strncmp(command_msg, "receiveAudio", strlen(command_msg)) == 0) {
	}
	else if (strncmp(command_msg, "receiveVideo", strlen(command_msg)) == 0) {
	}
	else if (strncmp(command_msg, "publish", strlen(command_msg)) == 0) {
		send_chunk(sfd, cs_id, 92, 20, 0);
		onPublish(sfd, transactionid);
		printf("responded to publish msg...\n");
	}
	else if (strncmp(command_msg, "seek", strlen(command_msg)) == 0) {
		printf("seeking command msg...\n");
	}
	else if (strncmp(command_msg, "pause", strlen(command_msg)) == 0) {
		printf("pausing command msg...\n");
	}
	free(command_msg);
}
// remember to put this back to system order (in our case it would be little endian)
// -- we don't need to because it just the value being calculated
void parse_set_chunk_size(unsigned char payload[], int size, unsigned int* MAX_CHUNK_SIZE) {
	*(MAX_CHUNK_SIZE) = payload[0]*(1 << 24) + payload[1]*(1 << 16) + payload[2]*(1 << 8) + payload[3];
}

void parse_abort_msg() {
	// no idea what to do with this
}

void parse_acknowledgement(unsigned char payload[], int size, unsigned int window_acknowledgement_size) {
	payload[0]*(1 << 24) + payload[1]*(1 << 16) + payload[2]*(1 << 8) + payload[3] == window_acknowledgement_size;
	// wtf do we do here
}

void parse_window_acknowledgement(unsigned char payload[], int size, unsigned int* window_acknowledgement_size) {
	*(window_acknowledgement_size) = payload[0]*(1 << 24) + payload[1]*(1 << 16) + payload[2]*(1 << 8) + payload[3];
}

void parse_set_peer_bandwidth() {
	// wtf do we do here
}

void parse_usr_msg() {
}

void parse_audio_msg(int sfd, unsigned char payload[], int size) {
}

void parse_video_msg(int sfd, unsigned char payload[], int size, unsigned int file_number) {
	int frametype = (payload[0] >> 4) & 255;
	int codecID = (payload[0] & 255);
	int avc_packet_type = payload[1];
	unsigned char composition_time[3] = {payload[2], payload[3], payload[4]};
	if (avc_packet_type == 0) {
		write_init(&(payload[5]), size-5);
	}
	else {
		// implement write_segment in streamsegmenter.c
		// write_segment(&(payload[5]), size-5);
	}
}

void parse_data_msg(int sfd, unsigned char payload[], int size) {
	FILE* fp = fopen("metadata.txt", "a");
	for (int i = 0; i < size; ++i) {
		char str[50];
		sprintf(str, "%i\n", payload[i]);
		fputs(str, fp);
	}
}


void parse_chunk_streams(int sfd) {
	// using longs because I don't know big message lengths can get
	// -- changed to ints because I am going to assume message lengths won't exceed 3 bytes
	unsigned int MAX_CHUNK_SIZE = 128;
	unsigned int bytes_received = 0;
	// msg length is only three bytes which means the max message length is 2^24-1
	unsigned int MSG_LENGTH = 0;
	// this will be our payload buffer
	// WE ONLY RESET PAYLOAD IF SIZE CHANGES, OTHERWISE WE ONLY HAVE TO RESET THE INDEX TO WHERE WE START INSERTING DATA; I AM ASSUMING THAT WORKS
	// THE RECV() FUNCTION WILL START INSERTING DATA INTO THE PAYLOAD AT THE SPECIFIED INDEX (BYTES_RECEIVED) AND OVERWRITE ANY BYTES THAT WAS THERE.
	unsigned char* payload = NULL;
	unsigned char MSG_TYPEID = 0;
	unsigned int CS_ID = 0;
	unsigned int WINDOW_ACKNOWLEDGEMENT_SIZE = 2e9;
	unsigned int TOTAL_BYTES = 0;
	unsigned int video_bytes = 0;
	unsigned int file_number = 0;
	while (1) {
		// used to see the first byte
		unsigned char first = 0;
		ssize_t _1 = recv(sfd, &first, 1, 0);
		if (_1 > 0) {
			//printf("FIRST: %i\n", first);
			unsigned char last_six_bits = first & 63;
			CS_ID = last_six_bits;
			// get entire basic header of chunk
			if (last_six_bits == 0) {
				chunk_basic_header_2_t second;
				ssize_t _2 = recv(sfd, &(second.second_byte), sizeof(second.second_byte), 0);
				CS_ID = second.second_byte + 64;
			}
			else if (last_six_bits == 1) {
				chunk_basic_header_3_t third;
				ssize_t _3 = recv(sfd, &(third.second_byte), sizeof(third.second_byte), 0);
				ssize_t _4 = recv(sfd, &(third.third_byte), sizeof(third.third_byte), 0);
				CS_ID = ((third.third_byte)*256 + (third.second_byte));
			}
			// don't need to do anything it if it type 1 basic header

			unsigned char first_two_bits = (first & 192) >> 6;
			// get message header of chunk
			/*
				Message header type 0 gives us the timestamp, message length, message type, message stream id.
				Keep in mind that the message length is the size of the message (not chunk size), which could be broken up into multiple chunks.
			*/
			if (first_two_bits == 0) {
				chunk_msg_header_0_t zero;
				get_msg_header(sfd, 0, &zero);
				MSG_LENGTH = get_size(zero.msg_length);
				MSG_TYPEID = zero.msg_typeid;
				// frees up any heap memory we used before
				free(payload);
				// initialize payload to new msg length
				payload = (unsigned char*)malloc((size_t)MSG_LENGTH);
			}
			// message header type 1
			else if (first_two_bits == 1) {
				chunk_msg_header_1_t one;
				get_msg_header(sfd, 1, &one);
				// maybe?
				MSG_LENGTH = get_size(one.msg_length);
				MSG_TYPEID = one.msg_typeid;
				// frees up any heap memory we used before
				free(payload);
				// initialize payload to new msg length
				payload = (unsigned char*)malloc((size_t)MSG_LENGTH);
			}
			// message header type 2
			else if (first_two_bits == 2) {
				chunk_msg_header_2_t two;
				get_msg_header(sfd, 2, &two);
			}
			// message header type 3
			// we don't need to do anything if it is a type 3 message header
			// just grab payload
			// -- what about the extended timestamp?

			// get payload and check if MSG_LENGTH-bytes_received or how much we need to grab, is greater than MAX_CHUNK_SIZE
			if (MSG_LENGTH - bytes_received > MAX_CHUNK_SIZE) {
				// we grab the MAX_CHUNK_SIZE
				ssize_t pl_rec_size = recv(sfd, payload+bytes_received, MAX_CHUNK_SIZE, 0);
				bytes_received += MAX_CHUNK_SIZE;
			}
			else {
				if (MSG_LENGTH - bytes_received == 0) {
					printf("msg length minus bytes received is zero. error\n");
					exit(EXIT_FAILURE);
				}
				// we grab the remainder
				ssize_t pl_rec_size = recv(sfd, payload+bytes_received, MSG_LENGTH - bytes_received, 0);
				bytes_received += (MSG_LENGTH - bytes_received);
			}
		}
		else {
			printf("first byte not read!\n");
			printf("video bytes: %i\n", video_bytes);
			exit(EXIT_FAILURE);
		}
		if (payload == NULL) {
			printf("payload pointer is null: %i\n", bytes_received);
			exit(EXIT_FAILURE);
		}
		if (bytes_received > MSG_LENGTH) {
			printf("bytes_received over msg_length: %i\n", bytes_received);
			exit(EXIT_FAILURE);
		}
		// bytes received serves as the index in our payload buffer, but if it equals MSG_LENGTH, that means that the index is out of bounds for our buffer
		// which also means that the payload has been filled
		if (bytes_received == MSG_LENGTH) {
			if (MSG_TYPEID == 20) {
				printf("parsing command msg...\n");
				parse_command_msg(sfd, CS_ID, payload, MSG_LENGTH, WINDOW_ACKNOWLEDGEMENT_SIZE);
			}
			else if (CS_ID == 2 && MSG_TYPEID >= 1 && MSG_TYPEID <= 6) {
				switch (MSG_TYPEID) {
					case 1: {
						parse_set_chunk_size(payload, MSG_LENGTH, &MAX_CHUNK_SIZE);
						break;
					}
					case 2: {
						parse_abort_msg();
						printf("abort msg...\n");
						exit(EXIT_FAILURE);
						break;
					}
					case 3: {
						parse_acknowledgement;
						printf("acknowledgement protocol control message\n");
						exit(EXIT_FAILURE);
						break;
					}
					case 4: {
						parse_usr_msg;
						printf("user event msg...\n");
						exit(EXIT_FAILURE);
						break;
					}
					case 5: {
						parse_window_acknowledgement;
						printf("window acknowledgement protocol cntrl msg...\n");
						exit(EXIT_FAILURE);
						break;
					}
					case 6: {
						parse_set_peer_bandwidth;
						printf("set peer bandwidth prtcl cntrl msg...\n");
						exit(EXIT_FAILURE);
						break;
					}
				}
			}
			else if (MSG_TYPEID == 8) {
				parse_audio_msg(sfd, payload, MSG_LENGTH);
			}
			else if (MSG_TYPEID == 9) {
				video_bytes += MSG_LENGTH;
				parse_video_msg(sfd, payload, MSG_LENGTH, file_number++);
			}
			else if (MSG_TYPEID == 18) {
				parse_data_msg(sfd, payload, MSG_LENGTH);
			}
			else {
				printf("you didn't account for msg type: %i, msg length: %i, first: %i\n", MSG_TYPEID, MSG_LENGTH, first);
				exit(EXIT_FAILURE);
			}
			TOTAL_BYTES += bytes_received;
			bytes_received = 0;
		}
	}
}

// handshake doesn't have different endianness considered
int handshake(int new_sfd) {
	/*
		The handshake begins with the client sending the C0 and C1 chunks.
		The client MUST wait until S1 has been received before sending C2.
		The client MUST wait until S2 has been received before sending any
		other data.
		The server MUST wait until C0 has been received before sending S0 and
		S1, and MAY wait until after C1 as well. The server MUST wait until
		C1 has been received before sending S2. The server MUST wait until
		C2 has been received before sending any other data.
	*/
	//printf("initiating handshake\n");
	packet_0_t* c0 				= (packet_0_t*)malloc(sizeof(packet_0_t));
	packet_1_t* c1 				= (packet_1_t*)malloc(sizeof(packet_1_t));
	packet_2_t* c2 				= (packet_2_t*)malloc(sizeof(packet_2_t));
	// reads from new sfd. recv() is usually used for tcp connections or connection based sockets
	ssize_t c0_size_version 	= recv(new_sfd, &(c0->version), sizeof(c0->version), 0) != sizeof(c0->version);

	//printf("c0 version: %u\n", c0->version);
	
	if ((c0->version >= 0 && c0->version <= 2) || 
		(c0->version >= 4 && c0->version <= 31) || 
		(c0->version >= 32 && c0->version <= 255)) {
		printf("version is not supported\n");
		if (close(new_sfd) != 0) {
			printf("socket failed to close after client requested version not supported\n");
		}
		return 0;
	}
	//---------------------------------------------

	// should receive c1 packet
	ssize_t c1_size_zero 		= recv(new_sfd, &(c1->zero), 4, 0); 
	ssize_t c1_size_time 		= recv(new_sfd, &(c1->time), 4, 0); 
	ssize_t c1_size_random 		= recv(new_sfd, &(c1->randombytes), 1528, 0);	

	//printf("c1 time byte: %u\n", c1->time);
	//---------------------------------------------

	packet_0_t* s0 				= (packet_0_t*)malloc(sizeof(packet_0_t));
	s0->version = 3;
	// sends s0 packet
	ssize_t s0_size_version 	= send(new_sfd, &(s0->version), 1, 0);

	//---------------------------------------------
	packet_1_t* s1 				= (packet_1_t*)malloc(sizeof(packet_1_t));
	// this field contains a timestamp, which SHOULD be used as the epoch for all future chunks sent from this endpoint.
	s1->time = c1->time;
	// funny how this doesn't have to be all zeroes xD
	// c2 time2's value is equal to this and according to the spec, time2 has to be equal to the previous packet's timestamp value (c1 or s1)
	// which means the client is using this as s1's timestamp
	s1->zero = 0;
	// c2 rand data must match s1 and s2 rand data must match c1
	for (int i = 0; i < 1528; ++i) {
		s1->randombytes[i] = 1;
	}
	// sends s1 packet
	ssize_t s1_size_time 		= send(new_sfd, &(s1->time), sizeof(s1->time), 0);
	ssize_t s1_size_zero 		= send(new_sfd, &(s1->zero), sizeof(s1->zero), 0);	
	ssize_t s1_size_random 		= send(new_sfd, &(s1->randombytes), sizeof(s1->randombytes), 0);
	//---------------------------------------------

	// sends s2 packet
	packet_2_t* s2 = (packet_2_t*)malloc(sizeof(packet_2_t));
	s2->time = c1->time;
	s2->time2 = c1->time;
	ssize_t s2_size_time 		= send(new_sfd, &(s2->time), sizeof(s2->time), 0);
	ssize_t s2_size_time2 		= send(new_sfd, &(s2->time2), sizeof(s2->time2), 0);
	ssize_t s2_size_random 		= send(new_sfd, &(s2->randombytes), sizeof(s2->randombytes), 0);
	//---------------------------------------------

	// should receive c2 packet
	ssize_t c2_size_time 		= recv(new_sfd, &(c2->time), 4, 0);	
	ssize_t c2_size_time2 		= recv(new_sfd, &(c2->time2), 4, 0);
	ssize_t c2_size_random 		= recv(new_sfd, &(c2->randombytes), 1528, 0);	

	for (int i = 0; i < 1528; ++i) {
		if (c2->randombytes[i] != s1->randombytes[i]) {
			printf("not equal\n");
			exit(EXIT_FAILURE);
		}
	}
	return 1;
}

int main(){
	struct sockaddr_in* address = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	// sets what type of ip addresses (IPv4 or IPv6)
	address->sin_family 			= AF_INET;
	// host to network long
	address->sin_addr.s_addr 		= htonl(0);
	// host to network short
	address->sin_port 				= htons(1935);
	int socket_file_descriptor 		= socket(AF_INET, SOCK_STREAM, 0);
	// BIND WASN'T WORKING BECAUSE SIZEOF() HAD A POINTER SIZE NOT THE ACTUAL SIZE OF THE STRUCT (EX: SIZEOF(ADDRESS), ADDRESS IS A POINTER; WHEREAS SIZEOF(*ADDRESS), IS ACTUALLY THE SIZE OF THE STRUCT)
	// binds an address to the socket struct/name
	int bind_code 					= bind(socket_file_descriptor, (struct sockaddr*)address, sizeof(struct sockaddr_in));
	// listen() marks a socket as a socket that will be listening for incoming connections using accept()
	int listen_code 				= listen(socket_file_descriptor, BACK_LOG);
	printf("Server listening on port 1935\n");
	socklen_t len = sizeof(struct sockaddr_in);
	// accept() will block the caller process until a connection is found. Once a connection is found, accept returns a new socket file descriptor.
	int new_sfd  = accept(socket_file_descriptor, (struct sockaddr*)address, &len);
	close(socket_file_descriptor);
	// rtmp handshake
	if (new_sfd != -1) {
		if (handshake(new_sfd)) {
			parse_chunk_streams(new_sfd);
		}
	}
	if (socket_file_descriptor == -1 || bind_code == -1 || listen_code == -1 || new_sfd == -1) {
		printf("ERROR IN MAIN");
		exit(EXIT_FAILURE);
	}
	close(socket_file_descriptor);
	close(new_sfd);
	return 0;
}
