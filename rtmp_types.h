#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// -- TURNS OUT BIT FIELDS ARE IMPLEMENTATION DEFINED ( we could use shifting and bit masking but we aren't working with individual bits or accessing specific bytes, just data members so this should be fine ) T_T. - 3/15/21 6:40 ish am.

// -- WAIT, we really need to think about how different computers and platforms may implement byte ordering differently, because we don't know how different compilers orders data
// -- we need to change how our structs are implemented. WE ARE NOT USING BIT FIELDS ANYMORE, and we have to read each data member individually instead of read data directly into a struct ( mainly because structs could have some unintended padding in between data members which could make us lose data ).

// it is better to send each data member individually so that data is lost. (endianness differences or padding or alignment issues, etc)
#define PACKET_0_T_SIZE 1
#define PACKET_1_T_SIZE 1536
#define PACKET_2_T_SIZE 1536
#define CHUNK_BASIC_HEADER_1_T_SIZE 1
#define CHUNK_BASIC_HEADER_2_T_SIZE 2
#define CHUNK_BASIC_HEADER_3_T_SIZE 3
#define CHUNK_MSG_HEADER_0_T_TIMESTAMP_SIZE 11
#define CHUNK_MSG_HEADER_1_T_TIMESTAMP_SIZE 7
#define CHUNK_MSG_HEADER_2_T_TIMESTAMP_SIZE 3
#define DEFAULT_CHUNK_SIZE 128
// handshake packets
typedef struct packet_0_t {
	// version specified by the RTMP 1.0 Specification is 3.
	// a server that does not recognize the client's requested server should respond with 3.
	// version is 1 byte
	unsigned char version;
} packet_0_t;

typedef struct packet_1_t {
	// timestamps in RTMP are given as an integer number of milliseconds relative to an unspecified epoch.
	// because timestamps are 32 bits, they roll over every 49 days, 17 hours, 2 minutes, and 47.296 seconds.
	unsigned int time;
	// zero fields must be all zeroes
	unsigned int zero;
	unsigned char randombytes[1528];
} packet_1_t;

typedef struct packet_2_t {
	unsigned int time;
	unsigned int time2;
	unsigned char randombytes[1528];
} packet_2_t;
//------------------------------------


typedef char msg_header_zero[11];


/*
Each chunk consists of a header and data. The header itself has
three parts:
+--------------+----------------+--------------------+--------------+
| Basic Header | Message Header | Extended Timestamp | Chunk Data |
+--------------+----------------+--------------------+--------------+
|                                                                 |
|<------------------- Chunk Header ----------------->|
Chunk Format

The Extended Timestamp field is used to encode timestamps or
timestamp deltas that are greater than 16777215 (0xFFFFFF); that is,
for timestamps or timestamp deltas that don’t fit in the 24 bit
fields of Type 0, 1, or 2 chunks. This field encodes the complete
32-bit timestamp or timestamp delta. The presence of this field is
indicated by setting the timestamp field of a Type 0 chunk, or the
timestamp delta field of a Type 1 or 2 chunk, to 16777215 (0xFFFFFF).
This field is present in Type 3 chunks when the most recent Type 0,
1, or 2 chunk for the same chunk stream ID indicated the presence of
an extended timestamp field.

*/

typedef struct chunk_basic_header_1_t {
	// format and cs_id can be in one byte, we just gotta read the first two bits and then the last six bits
	unsigned char first_byte;
} chunk_basic_header_1_t;
/*
Chunk stream IDs 64-319 can be encoded in the 2-byte form of the
header. ID is computed as (the second byte + 64).
 0                   1
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|fmt|     0     | cs id - 64   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
Chunk basic header 2
*/
typedef struct chunk_basic_header_2_t {
	// fmt and cs_id are in first byte
	unsigned char first_byte;
	// cs id - 64 is in second byte
	unsigned char second_byte;
} chunk_basic_header_2_t;
/*
Chunk stream IDs 64-65599 can be encoded in the 3-byte version of
this field. ID is computed as ((the third byte)*256 + (the second
byte) + 64).
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|fmt| 1          |        cs id - 64            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
Chunk basic header 3
*/
typedef struct chunk_basic_header_3_t {
	unsigned char first_byte;
	unsigned char second_byte;
	unsigned char third_byte;
} chunk_basic_header_3_t;

/*
	protocol control messages { 1, 2, 3, 5, 6 } are represented by the msg type ids and are used by the RTMP Chunk Stream.
	these protocol control messages MUST have message stream ID 0 and be sent in chunk stream ID 2.
	protocol control message 1 "Set Chunk Size"
		is used to notify the peer of a new maximum chunk size.

		Maximum chunk size defaults to 128 bytes, but the client or the server can change this value, and updates its peer using this message. 
		The maximum chunk size SHOULD be at least 128 bytes, and MUST be at least 1 byte.
	
	protocol control message 2 "Abort Message"
		is used to notify the peer if it is waiting ofr chunks to complete a message, then to discord the partially received message over a chunk stream.
		The peer receives the chunk stream ID as this protocol message's payload.
	
	protocol control message 3 "Acknowledgement"
		The client or the server MUST send an acknowledgement to the peer after receiving bytes equal to the window size. 
		The window size is the maximum number of bytes that the sender sends without receiving acknowledgement from the receiver.
		This message specifies the sequence number, which is the number of the bytes received so far.

	protocol control message 5 "Window acknowledgement size"
		client/server sends this message to inform the peer of the window size to use between sending acknowledgements. 
		The sender expects acknowledgements from its peer after the sender sends window size bytes.
		The receiving peer MUST send an acknowledgement after receiving the indicated number of bytes since the last acknowledgement was sent, 
		or from the beginning of the session if no acknowledgement has yet been sent.

	protocol control message 6 "set peer bandwidth"
		client/server sends this message to limit the output bandwidth of its peer.
		The peer receiving this message limits its output bandwidth by limiting the amount of sent but unacknowledged data to the window size indicated in this message.
		The peer receiving this message SHOULD respond with a Window acknowledgement size message if the winodw size is different from the last one sent to the sender of this message

	limit type is one of the following values:
		0 - Hard: the peer SHOULD lmit its output bandwidth to the indicated window size
		1 - soft: the peer SHOULD lmit its output bandwidth to the window indicated in this message or the limit already in effect, which ever is smaller.
		2 - dynamic: if the previous limit type was hard, treat this message as though it was marked hard, otherwise ignore this message.
		
		
*/
typedef struct set_chunk_size {
	// bit zero from the left MUST be zero
	unsigned int chunk_size;
} set_chunk_size;

typedef struct abort_message {
	unsigned int abort_payload_cs_id;
} abort_message;

typedef struct acknowledgement {
	unsigned int sequence_num;
} acknowledgement;

typedef struct window_acknowledgement_size {
	unsigned int window_size_byte;
} window_acknowledgement_size;

typedef struct set_peer_bandwidth {
	unsigned int window_size_byte;
	unsigned char limit_type;
} set_peer_bandwidth;
// ------------------------------------------------
typedef struct chunk_msg_header_0_t {
	unsigned char timestamp[3];
	unsigned char msg_length[3];
	unsigned char msg_typeid;
	unsigned int msg_streamid;
} chunk_msg_header_0_t;

typedef struct chunk_msg_header_1_t {
	unsigned char timestamp_delta[3];
	unsigned char msg_length[3];
	unsigned char msg_typeid;
} chunk_msg_header_1_t;

typedef struct chunk_msg_header_2_t {
	unsigned char timestamp_delta[3];
} chunk_msg_header_2_t;

typedef struct chunk_msg_header_3_t {
} chunk_msg_header_3_t;

typedef struct extended_timestamp {
	// ints are 4 bytes default
	unsigned int extend_timestamp;
} extended_timestamp;
/*
	RTMP Message format

	the RTMP message has two parts, a header and its payload.

	The header part:
		message type: one byte field to represent the message type. a range of type IDs (1-6) are reserved for protocol control messages.

		length: three byte field that represents the size of the payload in bytes. It is set in big-endian format.

		timestamp: 4 byte field that contains a timestamp of the message. the four bytes are packed in the big-endian order.

		message stream id: three byte field that identifies the stream of the message. these bytes are set in big endian.

	message payload:
		other part of the message is the payload, which is the actual data contained in the message  (audio or video).

	7.1. Types of Messages
			The server and the client send messages over the network to
			communicate with each other. The messages can be of any type which
			includes audio messages, video messages, command messages, shared
			object messages, data messages, and user control messages.
	
	User Control messages (4)

	RTMP uses message type ID 4 for User Control messages. These messages contain infomration used by the RTMP streaming layer.
	Protocol messages with IDs 1, 2, 3, 5, 6 are used by the RTMP Chunk Stream protocol.

	User Control messages should use message stream ID 0 (known as the control stream) and,
	when sent over RTMP Chunk Stream, be sent on chunk stream ID 2. User Control messages are effective at the point they are received in the stream; their timestamps are ignored.

	The client or the server sends this message to notify the peer about the user control events. 

*/

typedef struct message_header {
	unsigned char msg_type;
	unsigned char payload_length;
	unsigned int timestamp;
	unsigned char stream_id;
} message_header;

typedef struct usr_control_msg {
	unsigned short event_type;
	// this doesn't include event data which can fit into a 128 byte chunk
} usr_control_msg;

unsigned int get_current_time() {
	time_t rawtime;
	struct tm * timeinfo; 
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	return 1000*(60*60*(timeinfo->tm_hour)+60*(timeinfo->tm_min)+(timeinfo->tm_sec));
}
