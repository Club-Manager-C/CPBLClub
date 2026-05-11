#ifndef DB_H
#define DB_H

#include <windows.h>
#include <mysql.h>

void init_db(MYSQL **conn);
int check_login(MYSQL *conn, const char *id, const char *pw);
int check_id_duplicate(MYSQL *conn, const char *id);
int check_nickname_duplicate(MYSQL *conn, const char *nickname);
int check_student_id_duplicate(MYSQL *conn, const char *student_id);
int register_user(MYSQL *conn, const char *id, const char *pw,
                  const char *nickname, const char *student_id,
                  const char *name, const char *major, const char *phone);
void close_db(MYSQL *conn);

#endif // DB_H
