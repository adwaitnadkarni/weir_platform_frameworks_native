#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <utils/Log.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <inttypes.h>
#define WEIR_BUFFER_SIZE 256000
#define __STDC_FORMAT_MACROS

int weir_fd = -1;
char weir_buffer[WEIR_BUFFER_SIZE];
static pthread_t waiterThread;
int weirHookActive = 1;

//const int WEIR_MGR_ALLOW = 0;
//const int WEIR_MGR_DENY = 1;

const int DEBUG = 0;

ssize_t writeLine(int sockd, char* buffer, size_t n) {
    size_t      nleft;
    ssize_t     nwritten;

    nleft  = n;

    while ( nleft > 0 ) {
	if ( (nwritten = write(sockd, buffer, nleft)) <= 0 ) {
	    if ( errno == EINTR )
		nwritten = 0;
	    else
		return -1;
	}
	nleft  -= nwritten;
	buffer += nwritten;
    }

    return n;
}

ssize_t readLine(int sockd, char* buffer, size_t maxlen) {
    ssize_t n, rc;
    char    c;

    for ( n = 1; n < maxlen; n++ ) {

	if ( (rc = read(sockd, &c, 1)) == 1 ) {
	    *buffer++ = c;
	    if ( c == '\n' )
		break;
	}
	else if ( rc == 0 ) {
	    if ( n == 1 )
		return 0;
	    else
		break;
	}
	else {
	    if ( errno == EINTR )
		continue;
	    return -1;
	}
    }

    *buffer = 0;
    return n;
}

void weirConnect() {
	// Initialize the weir socket
	char* name = "WEIR";
	struct sockaddr_un addr;
	size_t namelen = strlen(name);
	int rc;
	int weir_error = 0;

	//LOGI("CameraService: UID: %i", getuid());
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_LOCAL;
	addr.sun_path[0] = 0;
	//addr.sun_path[1] = '\0';
	memcpy(addr.sun_path + 1, name, namelen);
	if ((weir_fd = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1) {
	    ALOGE("Socket error while creating WEIR socket");
	    return;
	}
	if ((connect(weir_fd, (struct sockaddr*) &addr, namelen + offsetof(struct sockaddr_un, sun_path) + 1)) == -1) {
	    ALOGE("Connection error while connecting to WEIR socket");
	    close(weir_fd);
	    weir_fd = -1;
	}
}

void weirDisconnect() {
	close(weir_fd);
	weir_fd = -1;
}

int weirQuery(char* args) {

	//ALOGD("WEIR: weirQuery: %s;%s;%s!", source, args, events);
	int weir_denied = 0;

	if (weir_fd == -1) {
		weirConnect();
	}

	if (weir_fd != -1) {
		char* emptyEvents = "";
		memset(weir_buffer, 0, sizeof(weir_buffer));
		sprintf(weir_buffer, "%s;\n", args);
		writeLine(weir_fd, weir_buffer, strlen(weir_buffer));
		//memset(weir_fd, 0, sizeof(buf));
		readLine(weir_fd, weir_buffer, WEIR_BUFFER_SIZE);
		weir_denied=atoi(weir_buffer);
	} else {
		ALOGD("WEIR: weirQuery: Socket not available! Allowing!");
	}

	return weir_denied;
}



