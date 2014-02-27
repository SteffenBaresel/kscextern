#ifndef LDAP_H
#define LDAP_H

#include <string>
#include "../lib/list.h"

using namespace std;

class LdapResult {
	friend class Ldap;
	private:
		string id;
		string name;
		List groups;
	public:
		string getUserId();
		string getUserName();
		int getGroupCount();
		string getGroup(int idx);
		string base64_encode(unsigned char const* , unsigned int len);
		string base64_decode(string const& s);
		bool isGroup(const string name);
};

class Ldap {
	private:
		string name;
		string password;
		string host;
		int port;
		string base;
		int version;
		int auth;
		int scope;
		List filters;
	public:
		Ldap();
		void setName(const string name);
		void setPassword(const string password);
		void setHost(const string host);
		void setPort(int port);
		void setBase(const string base);
		void setVersion(int version);
		void setAuth(int auth);
		void setScope(int scope);
		void addFilter(const string filter);
		LdapResult *search(const string user);
};

#endif
