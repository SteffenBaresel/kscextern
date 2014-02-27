CC := gcc
CFLAGS := -O3 -Wall -I/usr/local/include
#CFGLAGS += -ggdb3
LIB := /usr/local/lib/liblac.a
POSTGRESQL := -I$(shell pg_config --includedir-server) -I$(shell pg_config --includedir) -L$(shell pg_config --libdir) -lpq
all:
	$(CC) $(CFLAGS) -o export2csv export2csv.c database.c $(LIB) $(POSTGRESQL)

