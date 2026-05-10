#ifndef DB_H
#define DB_H

#include <windows.h>
#include <mysql.h>

void init_db(MYSQL **conn);
int check_login(MYSQL *conn, const char *id, const char *pw);
int check_id_duplicate(MYSQL *conn, const char *id);
int register_user(MYSQL *conn, const char *id, const char *pw, const char *nickname);
void close_db(MYSQL *conn);

#endif // DB_H
