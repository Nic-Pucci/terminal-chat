/* Nic Pucci
 * TERMINAL-CHAT IMPLEMENTATION
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "List.h"

const char *PROGRAM_NAME_FIRST_ARG = "terminal-chat";
const int MESSAGE_MAX_SIZE = 2048;

const int FAILED_SOCKET_FD = -1; // must be -1 because that is the error code returned by socket ()
const int FAILED_SENDING_MESSAGE = -1;
const int SUCCESS_SENDING_MESSAGE = 1;

const char REMOTE_TERMINAL_LABEL [] = "\nRemote: ";
const char USER_LEFT_CHAT_MESSAGE [] = "!";
const char REMOTE_LEFT_CHAT_RESPONSE [] = "[User left the chat]";
const char SESSION_STARTED_MESSAGE [] = "SESSION STARTED: Press RETURN KEY to send message. Enter '!' to exit the session.";
const char SESSION_ENDED_MESSAGE [] = "SESSION ENDED: [Disconnected]";

const char DEFAULT_TERMINAL_TEXT_COLOR [] = "\033[0m\n"; // default color by system
const char REMOTE_LABEL_TEXT_COLOR [] = "\033[1;34m"; // bold blue
const char REMOTE_MESSAGE_TEXT_COLOR [] = "\033[0;34m"; // blue
const char SESSION_STARTED_TEXT_COLOR [] = "\033[1;32m"; // bold green
const char SESSION_ENDED_TEXT_COLOR [] = "\033[1;31m"; // bold red

char *receivePort = "-1";
char *sendHostName = "-1"; // e.g. localhost = "127.0.0.1"
char *sendPort = "-1";

int receiveSocketFD = -1;
int sendSocketFD = -1;

LIST *sendMessagesList;
LIST *printMessagesList;

pthread_t sendThread;
pthread_t recvThread;
pthread_t inputThread;
pthread_t printingThread;

pthread_mutex_t sendMessagesListLock;
pthread_cond_t messageReadyToSendCondition;
pthread_cond_t messageSentCondition;

pthread_mutex_t printMessagesListLock;
pthread_cond_t messageReadyToPrintCondition;
pthread_cond_t messagePrintedCondition;

int StrEqual ( const char* str1 , const char* str2 ) {
	if ( !str1 || !str2 ) {
		return 0;
	}

	int comp = ( strcmp ( str1 , str2 ) == 0 );
	return comp;
}

void FreeMessages ( void *message ) {
	free ( ( char *) message );
}

void InitMutexConditionVars () {
	pthread_mutex_init ( &sendMessagesListLock , NULL );
	pthread_cond_init ( &messageReadyToSendCondition , NULL );
	pthread_cond_init ( &messageSentCondition , NULL );

	pthread_mutex_init ( &printMessagesListLock , NULL );
	pthread_cond_init ( &messageReadyToPrintCondition , NULL );
	pthread_cond_init ( &messagePrintedCondition , NULL );
}

void InitReceiveSocketFD () {
	int portNum = atoi ( receivePort );

	// 1. Create socket
	receiveSocketFD = socket ( AF_INET , SOCK_DGRAM , 0 );
	if ( receiveSocketFD < 0 ) {
		perror ( "cannot create socket" );
		receiveSocketFD = FAILED_SOCKET_FD;
	}

	// 2. Identify/name and the socket
	struct sockaddr_in receiveAddr;

	memset ( ( char *) &receiveAddr , 0 , sizeof ( receiveAddr ) );
	receiveAddr.sin_family = AF_INET;
	receiveAddr.sin_addr.s_addr = htonl ( INADDR_ANY );
	receiveAddr.sin_port = htons ( portNum );

	// bind the name to socket
	int b = bind ( 
		receiveSocketFD , 
		( struct sockaddr *) &receiveAddr , 
		sizeof ( receiveAddr ) 
	);

	if ( b < 0 ) {
		perror ( "bind failed" );
		receiveSocketFD = FAILED_SOCKET_FD;
	}
}

void InitSendSocketFD () {
	struct addrinfo hints;
	memset ( &hints , 0 , sizeof ( hints ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	struct addrinfo *servinfo;
	int err = getaddrinfo (
		sendHostName,
		sendPort,
		&hints,
		&servinfo
	);
	if ( err != 0 ) {
		printf ( "error %d : %s \n" , err, gai_strerror ( err ) );
		sendSocketFD = FAILED_SOCKET_FD;
		return;
	}

	struct addrinfo *p;
	for ( p = servinfo ; p != NULL ; p = p -> ai_next ) {
		sendSocketFD = socket (
			p -> ai_family ,
			p -> ai_socktype ,
			p -> ai_protocol
		);

		if ( sendSocketFD != -1 ) {
			break;
		}
	}

	if ( !p ) {
		fprintf ( stderr , "talker: failed to create socket\n" );
		sendSocketFD = FAILED_SOCKET_FD;
	}

	int c = connect ( sendSocketFD , p -> ai_addr , p -> ai_addrlen );
	if ( c < 0 ) {
		perror ( "connection failed" );
		sendSocketFD = FAILED_SOCKET_FD;
	}

	freeaddrinfo ( servinfo );
}

void CleanUp () {
	pthread_cond_destroy ( &messageReadyToSendCondition );
	pthread_cond_destroy ( &messageSentCondition );
	pthread_cond_destroy ( &messageReadyToPrintCondition );
	pthread_cond_destroy ( &messagePrintedCondition );

	pthread_mutex_destroy ( &sendMessagesListLock );
	pthread_mutex_destroy ( &printMessagesListLock );

	ListFree ( sendMessagesList , &FreeMessages );
	ListFree ( sendMessagesList , &FreeMessages );

	close ( sendSocketFD );
	close ( receiveSocketFD );
}

void WriteToScreen ( const char *str ) {
	if ( !str ) {
		return;
	}

	write ( STDOUT_FILENO , str , strlen ( str ) );
}

void *RunScreenPrinting () {
	if ( !printMessagesList ) {
		return NULL;
	}

	WriteToScreen ( SESSION_STARTED_TEXT_COLOR );
	WriteToScreen ( SESSION_STARTED_MESSAGE );
	WriteToScreen ( DEFAULT_TERMINAL_TEXT_COLOR );

	for ( ;; ) {
		pthread_mutex_lock ( &printMessagesListLock );
		pthread_cond_wait( &messageReadyToPrintCondition , &printMessagesListLock );

		char *printMessage = ( char *) ListTrim ( printMessagesList );
		if ( !printMessage ) {
			continue;
		}

		int userQuitSessionMessage = StrEqual ( printMessage , USER_LEFT_CHAT_MESSAGE );
		int remoteLeftSessionMessage = StrEqual ( printMessage , REMOTE_LEFT_CHAT_RESPONSE );

		if ( !userQuitSessionMessage ) {
			WriteToScreen ( REMOTE_LABEL_TEXT_COLOR );
			WriteToScreen ( REMOTE_TERMINAL_LABEL );
			WriteToScreen ( REMOTE_MESSAGE_TEXT_COLOR );
			WriteToScreen ( printMessage );
			WriteToScreen ( DEFAULT_TERMINAL_TEXT_COLOR );
		}

		FreeMessages ( printMessage );

		pthread_cond_signal ( &messagePrintedCondition ); 
		pthread_mutex_unlock ( &printMessagesListLock );

		if ( userQuitSessionMessage || remoteLeftSessionMessage ) {
			break;
		}
	}

	WriteToScreen ( SESSION_ENDED_TEXT_COLOR );
	WriteToScreen ( SESSION_ENDED_MESSAGE );
	WriteToScreen ( DEFAULT_TERMINAL_TEXT_COLOR );

	return NULL;
}

void *RunReceiving () {
	if ( receiveSocketFD == FAILED_SOCKET_FD ) {
		return NULL;
	}

	if ( receiveSocketFD == FAILED_SOCKET_FD ) {
		return NULL;
	}

	if ( !printMessagesList ) {
		return NULL;
	}

	struct sockaddr_in remaddr; // remote address
	socklen_t addrlen = sizeof ( remaddr ); // length of address
	int recvlen; // # bytes received
	unsigned char receiveBuffer [ MESSAGE_MAX_SIZE]; // receive buffer

	for ( ;; ) {
		recvlen = recvfrom (
			receiveSocketFD , 
			receiveBuffer ,
			MESSAGE_MAX_SIZE , 
			0 , 
			( struct sockaddr *) &remaddr , 
			&addrlen 
		);

		if ( recvlen > 0 ) {
			receiveBuffer [ recvlen ] = 0; // remove whitespace chars

			char *receivedMessage = ( char *) malloc ( sizeof ( char ) * MESSAGE_MAX_SIZE );
			strncpy ( receivedMessage , ( char *) receiveBuffer , MESSAGE_MAX_SIZE );

			pthread_mutex_lock ( &printMessagesListLock );
			ListPrepend ( printMessagesList , ( void *) receivedMessage );

			pthread_cond_signal ( &messageReadyToPrintCondition );

			pthread_cond_wait( &messagePrintedCondition, &printMessagesListLock ); 
			pthread_mutex_unlock ( &printMessagesListLock );
		}
	}

	return NULL;
}

int SendMessage ( const char *message ) {
	if ( sendSocketFD == FAILED_SOCKET_FD ) {
		perror ( "Send Socket is not initialized: message failed to send" );
		return FAILED_SENDING_MESSAGE;
	}

	int numSentBytes = sendto ( 
		sendSocketFD , 
		message , 
		strlen ( message ) , 
		0 , 
		0 , // p -> ai_addr , 
		0 // p -> ai_addrlen
	);
	if ( numSentBytes == -1 ) {
		perror ( "message failed to send" );
		return FAILED_SENDING_MESSAGE;
	}

	return SUCCESS_SENDING_MESSAGE;
}

void *RunSending () {
	if ( !sendMessagesList ) {
		return NULL;
	}

	for ( ;; ) {
		pthread_mutex_lock ( &sendMessagesListLock );

		pthread_cond_wait( &messageReadyToSendCondition, &sendMessagesListLock );

		char *sendMessage = ( char *) ListTrim ( sendMessagesList );
		if ( sendMessage ) {
			int quitSessionMessage = StrEqual ( sendMessage , USER_LEFT_CHAT_MESSAGE );
			if ( quitSessionMessage ) {
				strncpy ( sendMessage , REMOTE_LEFT_CHAT_RESPONSE , MESSAGE_MAX_SIZE );
			}
		}

		SendMessage ( sendMessage );
		FreeMessages ( sendMessage );
			
		pthread_cond_signal ( &messageSentCondition ); 
		pthread_mutex_unlock ( &sendMessagesListLock );
	}

	return NULL;
}

void *RunUserInput () {
	char inputBuffer [ MESSAGE_MAX_SIZE ];
	int inputLength = 0;

	while ( ( inputLength = read ( STDIN_FILENO , inputBuffer , MESSAGE_MAX_SIZE ) ) > 0 )
	{
		char lastChar = inputBuffer [ inputLength - 1 ];
		if ( lastChar == 10 ) {
			inputBuffer [ inputLength - 1 ] = 0; // remove newline char
		}

		char *sendMessage = ( char *) malloc ( sizeof ( char ) * MESSAGE_MAX_SIZE );
		strncpy ( sendMessage , inputBuffer , MESSAGE_MAX_SIZE );
	
		pthread_mutex_lock ( &sendMessagesListLock );

		ListPrepend ( sendMessagesList , ( void * ) sendMessage );

		pthread_cond_signal ( &messageReadyToSendCondition );
		
		pthread_cond_wait( &messageSentCondition, &sendMessagesListLock ); 
		pthread_mutex_unlock ( &sendMessagesListLock );

		int quitSessionInput = StrEqual ( inputBuffer , USER_LEFT_CHAT_MESSAGE );
		if ( quitSessionInput ) {
			char *endSessionMessage = ( char *) malloc ( sizeof ( char ) * MESSAGE_MAX_SIZE );
			strncpy ( endSessionMessage , ( char *) &USER_LEFT_CHAT_MESSAGE , MESSAGE_MAX_SIZE );

			pthread_mutex_lock ( &printMessagesListLock );
			ListPrepend ( printMessagesList , ( void *) endSessionMessage );

			pthread_cond_signal ( &messageReadyToPrintCondition );

			pthread_cond_wait( &messagePrintedCondition, &printMessagesListLock ); 
			pthread_mutex_unlock ( &printMessagesListLock );
		}
	}

	return NULL;
}

int main ( int argc , char *argv [] ) 
{
	if ( argc < 5 ) {
		WriteToScreen ( "Incorrect amount of inputs. Please include the following arguments:\n");
		WriteToScreen ( "terminal-chat [your port number] [remote machine name] [remote port number]\n");
		exit ( -1 );
	}

	int correctFirstParameter = StrEqual ( PROGRAM_NAME_FIRST_ARG , argv [ 1 ] );
	if ( !correctFirstParameter ) {
		WriteToScreen ( "Incorrect first argument. The first input must be \"terminal-chat\"\n" );
		exit ( -1 );
	}

	receivePort = argv [ 2 ];
	sendHostName = argv [ 3 ];
	sendPort = argv [ 4 ];

	InitReceiveSocketFD ();
	if ( receiveSocketFD == FAILED_SOCKET_FD ) {
		WriteToScreen ( "ERROR: Receive Socket failed to be created" );
		exit ( -1 );
	}
	
	InitSendSocketFD ();
	if ( sendSocketFD == FAILED_SOCKET_FD ) {
		WriteToScreen ( "ERROR: Send Socket failed to be created" );
		exit ( -1 );
	}

	sendMessagesList = ListCreate ();
	if ( !sendMessagesList ) {
		WriteToScreen ( "Send Messages List wasn't created" );
		exit ( -1 );
	}

	printMessagesList = ListCreate ();
	if ( !printMessagesList ) {
		WriteToScreen ( "Print Messages List wasn't created" );
		exit ( -1 );
	}

	InitMutexConditionVars ();
	
	pthread_attr_t threadAttribute;
	pthread_attr_init ( &threadAttribute );
    pthread_attr_setdetachstate ( &threadAttribute , PTHREAD_CREATE_JOINABLE );

	pthread_create ( &printingThread , &threadAttribute , RunScreenPrinting , NULL );

	sleep ( 0.5f ); // allow time for the printing thread to print session start message before user input

	pthread_create ( &sendThread , &threadAttribute , RunSending , NULL );     
	pthread_create ( &recvThread , &threadAttribute , RunReceiving , NULL );
	pthread_create ( &inputThread , &threadAttribute , RunUserInput , NULL );

	pthread_join ( printingThread , NULL );

	pthread_cancel ( recvThread );
	pthread_cancel ( sendThread );
	pthread_cancel ( inputThread );

	pthread_join ( sendThread , NULL );
	pthread_join ( recvThread , NULL );
	pthread_join ( inputThread , NULL );
	
	CleanUp ();

	exit ( 0 );
}

