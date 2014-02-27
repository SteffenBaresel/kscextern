#include "mk.lib.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>

MK::MK() {
	connection = -1;
	result = new vector<string>();
	open();
}

MK::~MK() {
	close();
}

void MK::open() {
    connection = socket(PF_LOCAL, SOCK_STREAM, 0);
    struct sockaddr_un sa;
    sa.sun_family = AF_LOCAL;
    strncpy(sa.sun_path, MK_SOCKET_PATH, sizeof(sa.sun_path));
    if (0 > connect(connection, (const struct sockaddr *)&sa, sizeof(sockaddr_un))) {
		::close(connection);
		connection = -1;	
    } else 
	file = fdopen(connection, "r");
}

void MK::close() {
	closeResult();

	if (isConnection()) {
		if (file) {
			fclose(file);
		} else {
			::close(connection);
		}
	}

	connection = -1;
	file = NULL;
}

bool MK::isConnection() {
	return connection >= 0;
}

void MK::execute(const string query) {
    write(connection, query.c_str(), strlen(query.c_str()));
    write(connection, MK_SEPARATORS, strlen(MK_SEPARATORS));
    shutdown(connection, SHUT_WR);
}

void MK::closeResult() {
	if (result != NULL) delete(result);
	result = NULL;
}

bool MK::nextResult() {
	closeResult();
	result = new vector<string>;

    char line[65536];
    if (0 != fgets(line, sizeof(line), file)) {
		// strip trailing linefeed
		char *end = strlen(line) + line;
		if (end > line && *(end-1) == '\n') {
			*(end-1) = 0;
			--end;
		}

		char *scan = line;
		while (scan < end) {
			char *zero = scan;
			while (zero < end && *zero != '\001') zero++;
			*zero = 0;
			result->push_back(string(scan));
			scan = zero + 1;
		}
		return true;
    } else {
		return false;
	}
}

int MK::getResultCount() {
	return result->size();
}

string MK::getString(int id) {
	return (*result)[id];
}
