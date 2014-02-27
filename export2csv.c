#include <stdlib.h>
#include <stdio.h>
#include <la_datetime.h>
#include <la_directory.h>
#include <la_error.h>
#include <la_file.h>
#include <la_list.h>
#include <la_parameter.h>
#include <la_stringbuffer.h>
#include "database.h"

/* PRG */
#define PRG_TITLE "Kvasy System Control - Tools"
#define PRG_NAME "export2csv"
#define PRG_VERSION "1.2.4"
#define PRG_DATE "2012-2013"
#define PRG_AUTHOR "Stephan Laukien"

/* HEADER */
#define HEADER_AVAILABILITY "peavid;custsn;hostna;applln;svtyln;descr1;descr2;timeok;timewa;timecr;timeun;dwntim;utime;"
#define HEADER_PERFORMANCE "pemoid;custsn;hostna;applln;svtyln;descr1;descr2;usage;size;utime;"

/* SQL */
#define SQL_AVAILABILITY_NAME "SELECT DISTINCT custsn FROM export2csvavailability WHERE peavid > ?"
#define SQL_AVAILABILITY_DATA "SELECT * FROM export2csvavailability WHERE peavid > ? ORDER BY peavid DESC"
#define SQL_AVAILABILITY_CONFIG_GET "SELECT value FROM config WHERE key = 'export2csv.availability'"
#define SQL_AVAILABILITY_CONFIG_SET "INSERT INTO config (key) values ('export2csv.availability')"
#define SQL_AVAILABILITY_CONFIG_UPD "UPDATE config set value=? WHERE key='export2csv.availability'"
#define SQL_PERFORMANCE_NAME "SELECT DISTINCT custsn FROM export2csvperformance WHERE pemoid > ?"
#define SQL_PERFORMANCE_DATA "SELECT * FROM export2csvperformance WHERE pemoid > ? ORDER BY pemoid DESC"
#define SQL_PERFORMANCE_CONFIG_GET "SELECT value FROM config WHERE key = 'export2csv.performance'"
#define SQL_PERFORMANCE_CONFIG_SET "INSERT INTO config (key) values ('export2csv.performance')"
#define SQL_PERFORMANCE_CONFIG_UPD "UPDATE config set value=? WHERE key='export2csv.performance'"

/* global definition */
EXCEPTION *e;
PARAMETER *param;
DATABASE *dbo;
char *timestamp;
char *destination;

/* declaration */
void exception(int id, const char *msg, void *ptr);
void error(const char *message);
void copyright(void);
void usage(void);
void clear(void);
int main(int argc, char *argv[]) {

	copyright();

	/* init */
	e = NULL;
	param = NULL;
	dbo = NULL;
	timestamp = NULL;

	error_init();
	e = exception_new();
	exception_addCallback(e, exception, NULL);

	/* argument */
	if (argc != 2) {
		error("no properties");
		usage();
		return (EXIT_FAILURE);
	}

	/* filename */
	char *filename = argv[1];
	if (!file_exists(filename)) {
		error("file not found");
		return(EXIT_FAILURE);
	}

	/* parameter */
	param = parameter_new();
	parameter_loadFromFile(param, filename);

	/* destination */
	destination = parameter_get(param, "directory.destination");
	if (destination == NULL)
		destination = strdup("ksc");
	printf ( "DESTINATION:\t%s\n", destination );

	/* timestamp */
	timestamp = datetime_getTimestampAsString();

	/* init und fill database-object */
	dbo = database_new();
	if (dbo == NULL) {
		error("unable to get database-object");
		return(EXIT_FAILURE);
	}

	database_exception(dbo, e);
	database_load(dbo, param);
	database_open(dbo);

	char *res;
	int i, j;

	/* availability */
	printf ( "AVAILABILITY:\n" );
	STRINGBUFFER *peav_name;
	FILE *peav_file;
	unsigned long peav_id = 0;
	unsigned long peav_last = 0;
	unsigned int peav_count = 0;
	unsigned int peav_rows;
	unsigned int peav_cols;
	LIST *peav_list;
	char *peav_cust;

	/* read config */
	database_postgresql_execute(dbo->db, SQL_AVAILABILITY_CONFIG_GET);
	if (database_postgresql_getResultRowCount(dbo->db) == 1) {
		database_postgresql_nextResult(dbo->db);
		res = database_postgresql_getString(dbo->db, 0);
		if (res != NULL) {
			peav_last = atoi(res);
			free(res);
		}
	} else database_postgresql_execute(dbo->db, SQL_AVAILABILITY_CONFIG_SET);

	/* get list of all customers */
	peav_list = list_new();
	database_postgresql_execute(dbo->db, SQL_AVAILABILITY_NAME, peav_last);
	if (database_postgresql_getResultRowCount(dbo->db) == 0) peav_id = peav_last;
	while (database_postgresql_nextResult(dbo->db)) {
		res = database_postgresql_getString(dbo->db, 0);
		list_add(peav_list, res);
		free(res);
	}


	/* get and write data of the customer */
	for (j = 0; j < list_size(peav_list); ++j) {
		peav_cust = list_get(peav_list, j);
		printf ( "%30s ", peav_cust );

		peav_name = stringbuffer_new();
		stringbuffer_append(peav_name, destination);
		stringbuffer_append(peav_name, DIRECTORY_SEPARATOR_STRING);
		stringbuffer_append(peav_name, peav_cust);

		directory_create(peav_name->text);

		stringbuffer_append(peav_name, DIRECTORY_SEPARATOR_STRING);
		stringbuffer_append(peav_name, "peav.");
		stringbuffer_append(peav_name, timestamp);
		stringbuffer_append(peav_name, ".csv");
		peav_file = fopen(peav_name->text, "w");

		fprintf(peav_file, "%s\n", HEADER_AVAILABILITY);

		database_postgresql_execute(dbo->db, SQL_AVAILABILITY_DATA, peav_last);
		peav_rows = database_postgresql_getResultRowCount(dbo->db);
		peav_cols = database_postgresql_getResultColumnCount(dbo->db);
		while (database_postgresql_nextResult(dbo->db)) {
			if (peav_count > 0 && (peav_count % ((peav_rows / 25) + 1) == 0)) printf ( "." );
			if (peav_id == 0) {
				res = database_postgresql_getString(dbo->db, 0);
				peav_id = atoi(res);
				free(res);
			}
			for (i = 0; i < peav_cols; ++i) {
				res = database_postgresql_getString(dbo->db, i);
				if (res == NULL)
					fprintf(peav_file, ";");
				else {
					fprintf(peav_file, "%s;", res);
					free(res);
				}
			}
			fprintf(peav_file, "\n");
			++peav_count;
		}
		fclose(peav_file);
		stringbuffer_free(peav_name);
		free(peav_cust);
		printf ( "%d\n", peav_count );
	}

	database_postgresql_execute(dbo->db, SQL_AVAILABILITY_CONFIG_UPD, peav_id);

	/* performance */
	printf ( "PERFORMANCE:\n" );
	STRINGBUFFER *pemo_name;
	FILE *pemo_file;
	unsigned long pemo_id = 0;
	unsigned long pemo_last = 0;
	unsigned int pemo_count = 0;
	unsigned int pemo_rows;
	unsigned int pemo_cols;
	LIST *pemo_list;
	char *pemo_cust;

	/* read config */
	database_postgresql_execute(dbo->db, SQL_PERFORMANCE_CONFIG_GET);
	if (database_postgresql_getResultRowCount(dbo->db) == 1) {
		database_postgresql_nextResult(dbo->db);
		res = database_postgresql_getString(dbo->db, 0);
		if (res != NULL) {
			pemo_last = atoi(res);
			free(res);
		}
	} else database_postgresql_execute(dbo->db, SQL_PERFORMANCE_CONFIG_SET);

	/* get list of all customers */
	pemo_list = list_new();
	database_postgresql_execute(dbo->db, SQL_PERFORMANCE_NAME, pemo_last);
	if (database_postgresql_getResultRowCount(dbo->db) == 0) pemo_id = pemo_last;
	while (database_postgresql_nextResult(dbo->db)) {
		res = database_postgresql_getString(dbo->db, 0);
		list_add(pemo_list, res);
		free(res);
	}


	/* get and write data of the customer */
	for (j = 0; j < list_size(pemo_list); ++j) {
		pemo_cust = list_get(pemo_list, j);
		printf ( "%30s ", pemo_cust );

		pemo_name = stringbuffer_new();
		stringbuffer_append(pemo_name, destination);
		stringbuffer_append(pemo_name, DIRECTORY_SEPARATOR_STRING);
		stringbuffer_append(pemo_name, pemo_cust);

		directory_create(pemo_name->text);

		stringbuffer_append(pemo_name, DIRECTORY_SEPARATOR_STRING);
		stringbuffer_append(pemo_name, "pemo.");
		stringbuffer_append(pemo_name, timestamp);
		stringbuffer_append(pemo_name, ".csv");
		pemo_file = fopen(pemo_name->text, "w");

		fprintf(pemo_file, "%s\n", HEADER_PERFORMANCE);

		database_postgresql_execute(dbo->db, SQL_PERFORMANCE_DATA, pemo_last);
		pemo_rows = database_postgresql_getResultRowCount(dbo->db);
		pemo_cols = database_postgresql_getResultColumnCount(dbo->db);
		while (database_postgresql_nextResult(dbo->db)) {
			if (pemo_count > 0 && (pemo_count % ((pemo_rows / 25) + 1) == 0)) printf ( "." );
			if (pemo_id == 0) {
				res = database_postgresql_getString(dbo->db, 0);
				pemo_id = atoi(res);
				free(res);
			}
			for (i = 0; i < pemo_cols; ++i) {
				res = database_postgresql_getString(dbo->db, i);
				if (res == NULL)
					fprintf(pemo_file, ";");
				else {
					fprintf(pemo_file, "%s;", res);
					free(res);
				}
			}
			fprintf(pemo_file, "\n");
			++pemo_count;
		}
		fclose(pemo_file);
		stringbuffer_free(pemo_name);
		free(pemo_cust);
		printf ( "%d\n", pemo_count );
	}

	database_postgresql_execute(dbo->db, SQL_PERFORMANCE_CONFIG_UPD, pemo_id);

	/* free */
	clear();
	return (EXIT_SUCCESS);
}

void error(const char *message) {
	fprintf(stderr, "ERROR:\t\t%s\n\n", message);
	clear();
}

void copyright(void) {
	printf ( "\n" );
	printf ( "%s (%s) v.%s\n", PRG_TITLE, PRG_NAME, PRG_VERSION );
	printf ( "(c) %s by %s\n", PRG_DATE, PRG_AUTHOR );
	printf ( "\n" );
}

void usage(void) {
	printf ( "%s [PROPERTIES]\n", PRG_NAME );
	printf ( "\n" );
	printf ( "Properties file:\n" );
	printf ( "\tdirectory.destination\n" );
	printf ( "\tdatabase.type\n" );
	printf ( "\tdatabase.name\n" );
	printf ( "\tdatabase.host\n" );
	printf ( "\tdatabase.port\n" );
	printf ( "\tdatabase.schema\n" );
	printf ( "\tdatabase.user\n" );
	printf ( "\tdatabase.password\n" );
}

void exception(int id, const char *msg, void *ptr) {
	printf ( "ERROR:\t\t%s (%d)\n", msg, id );
	clear();
	exit(id);
}

void clear(void) {
	if (dbo != NULL) {
		database_close(dbo);
		database_free(dbo);
		dbo = NULL;
	}
	if (timestamp != NULL) {
		free(timestamp);
		timestamp = NULL;
	}
	if (destination != NULL) {
		free(destination);
		destination = NULL;
	}
	if (param != NULL) {
		parameter_free(param);
		param = NULL;
	}
	if (e != NULL) {
		exception_free(e);
		e = NULL;
	}
}
