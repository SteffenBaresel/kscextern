#include <stdlib.h>
#include <la_error.h>
#include "database.h"

DATABASE *database_new() {
	DATABASE *self = (DATABASE *) malloc(sizeof(DATABASE));
	if (self == NULL) {
		error_set(1, "no memory");
		return NULL;
	}

	self->db = NULL;
	self->host = NULL;
	self->port = -1;
	self->name = NULL;
	self->schema = NULL;
	self->user = NULL;
	self->password = NULL;

	return self;
}

void database_free(DATABASE *self) {
	database_close(self);

	if (self->host != NULL) {
		free(self->host);
		self->host = NULL;
	}
	self->port = -1;
	if (self->name != NULL) {
		free(self->name);
		self->name = NULL;
	}
	if (self->schema != NULL) {
		free(self->schema);
		self->schema = NULL;
	}
	if (self->user != NULL) {
		free(self->user);
		self->user = NULL;
	}
	if (self->password != NULL) {
		free(self->password);
		self->password = NULL;
	}

	database_postgresql_free(self->db);
	self->db = NULL;

	exception_delCallback(self->exception);
	self->exception = NULL;

	free(self);
	self = NULL;
}

void database_open(DATABASE *self) {
	self->db = database_postgresql_new();
	if (error_exists()) return;
	database_postgresql_setException(self->db, self->exception);

	database_postgresql_setHost(self->db, self->host);
	database_postgresql_setPort(self->db, self->port);
	database_postgresql_setName(self->db, self->name);
	database_postgresql_setUser(self->db, self->user);
	database_postgresql_setPassword(self->db, self->password);

	database_postgresql_open(self->db);
}

void database_close(DATABASE *self) {
	if (self->db != NULL) return;

	database_postgresql_close(self->db);
	database_postgresql_free(self->db);

	self->db = NULL;
}

void database_load(DATABASE *self, PARAMETER *param) {
	self->host = parameter_get(param, "database.host");
	char *port = parameter_get(param, "database.port");
	self->name = parameter_get(param, "database.name");
	self->schema = parameter_get(param, "database.schema");
	self->user = parameter_get(param, "database.user");
	self->password = parameter_get(param, "database.password");

	if (self->host == NULL) {
		error_set(2, "'database.host' not set");
		return;
	}
	if (port == NULL) {
		error_set(2, "'database.port' not set");
		return;
	} else {
		self->port = atoi(port);
		free(port);
	}
	if (self->name == NULL) {
		error_set(2, "'database.name' not set");
		return;
	}
	if (self->schema == NULL) {
		error_set(2, "'database.schema' not set");
		return;
	}
	if (self->user == NULL) {
		error_set(2, "'database.user' not set");
		return;
	}
	if (self->password == NULL) {
		error_set(2, "'database.password' not set");
		return;
	}

	printf ( "DATABASE:\t%s@%s (%s)\n", self->user, self->name, self->host );
}

void database_exception(DATABASE *self, EXCEPTION *e) {
	self->exception = e;
}
