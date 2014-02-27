#include "ldap.lib.h"
#include "../lib/common.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <ldap.h>

#define LINE_SIZE 256

Ldap::Ldap() {
//	this->name.clear();
//	this->password.clear();
//	this->host.clear();
	this->port = 389;
//	this->base.clear();
	this->version = LDAP_VERSION3;
	this->auth = LDAP_AUTH_SIMPLE;
	this->scope = LDAP_SCOPE_SUBTREE;
//	this->filters.delAll();
	this->addFilter("displayName");
	this->addFilter("memberOf");
}

void Ldap::setName(const string name) {
	this->name = name;
}

void Ldap::setPassword(const string password) {
	this->password = password;
}

void Ldap::setHost(const string host) {
	this->host = host;
}

void Ldap::setPort(int port) {
	this->port = port;
}

void Ldap::setBase(const string base) {
	this->base = base;
}

void Ldap::setVersion(int version) {
	this->version = version;
}

void Ldap::setAuth(int auth) {
	this->auth = auth;	
}

void Ldap::setScope(int scope) {
	this->scope = scope;	
}

void Ldap::addFilter(const string filter) {
	this->filters.add(filter);
}

static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

/* ldapsearch -x -h ads01.siv.de -p 389 -D "CN=icinga ad,OU=Special User,OU=SIV Benutzer,DC=siv,DC=de" -w "1JRpMOkq" -b "DC=siv,DC=de" -LLL "sAMAccountName=sbaresel" displayName memberOf */
LdapResult *Ldap::search(const string user) {
	string cmd;
	cmd += "ldapsearch -x";
	cmd += " -h " + this->host;
	cmd += " -p " + toString(this->port);
	cmd += " -D \"" + this->name + "\"";
	cmd += " -w " + this->password;
	cmd += " -b " + this->base;
	cmd += " -LLL \"sAMAccountName=" + user + "\"";
//	cmd += " -LLL \"sAMAccountName=bvoelcker\"";
//	cmd += " -LLL \"displayName=" + user + "\"";
	for (int i = 0; i < this->filters.size(); ++i) {
		cmd += " " + this->filters.get(i);
	}

//	cout << "CMD: " << cmd << endl;

	char buf[LINE_SIZE + 1];

	FILE *p;
	p = popen(cmd.c_str(), "r");
	if (p == NULL) {
		return NULL;
	}
	LdapResult *res = new LdapResult();
	res->id = user;
	string line;
	while (fgets(buf, LINE_SIZE, p)) {
		line = buf;
		line = trim(line, " \t\n\r");
		//line = res->base64_decode(line);
		//cout << "Line: " << line << "<br>" << endl;
		if (line.size() == 0 || line[0] == '#') continue;
		if (line.find("displayName: ") == 0) {
			res->name = line.substr(line.find(' ') + 1);
			continue;
		} else if (line.find("displayName:: ") == 0) {
			res->name = res->base64_decode(line.substr(line.find(' ') + 1));
			continue;
		}
		if (line.find("memberOf: ") == 0) {
			res->groups.add(line.substr(line.find(' ') + 1));
			continue;
		}
		//cout << "Line: " << line << endl;
	}
	pclose(p);

	return res;

}

string LdapResult::getUserId() {
	return id;
}

string LdapResult::getUserName() {
	return name;
}

int LdapResult::getGroupCount() {
	return groups.size();
}

string LdapResult::getGroup(int idx) {
	return groups.get(idx);
}

bool LdapResult::isGroup(const string name) {
	string id = "OU=" + name + ',';
	for (int i = 0; i < groups.size(); ++i) {
		if (groups.get(i).find(id) != string::npos) return true;
	}

	return false;
}

static const string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";


string LdapResult::base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
    string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
	char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
    	    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];
		i = 0;
        }
    }

    if (i)
    {
        for(j = i; j < 3; j++)
	    char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

	for (j = 0; (j < i + 1); j++)
    	    ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
    	    ret += '=';

    }
    return ret;
}

string LdapResult::base64_decode(string const& encoded_string) {
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    string ret;

    while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
	char_array_4[i++] = encoded_string[in_]; in_++;
        if (i ==4) {
    	    for (i = 0; i <4; i++)
        	char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
	    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

	    for (i = 0; (i < 3); i++)
        	ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j <4; j++)
	    char_array_4[j] = 0;

	for (j = 0; j <4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
	char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }

    return ret;
}
