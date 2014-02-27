#ifndef MK_H
#define MK_H

#include <cstdio>
#include <string>
#include <vector>

#define MK_SOCKET_PATH "/usr/local/icinga/var/rw/live"
#define MK_SEPARATORS "Separators: 10 1 2 3\n"
#define MK_LINE_SIZE 8192

using namespace std;

class MK {
	private:
		int connection;
		FILE *file;
		vector<string> *result;
	public:
		MK();
		~MK();
		void open();
		void close();
		void closeResult();
		bool isConnection();
		void execute(const string query);
		bool nextResult();
		int getResultCount();
		string getString(int id);
};

#endif
