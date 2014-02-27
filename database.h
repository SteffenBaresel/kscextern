#ifndef DATABASE_H
#define DATABASE_H

#include <la_database_postgresql.h>
#include <la_exception.h>
#include <la_parameter.h>

typedef struct {
	EXCEPTION *exception;
	DATABASE_POSTGRESQL *db;
	char *host;
	int port;
	char *name;
	char *schema;
	char *user;
	char *password;
} DATABASE;

DATABASE *database_new();
void database_free(DATABASE *self);
void database_open(DATABASE *self);
void database_close(DATABASE *self);
void database_load(DATABASE *self, PARAMETER *param);
void database_exception(DATABASE *self, EXCEPTION *e);

#endif
