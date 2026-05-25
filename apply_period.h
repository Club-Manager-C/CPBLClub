#ifndef CLUB_APPLY_PERIOD_H
#define CLUB_APPLY_PERIOD_H

#include <mysql.h>

void manage_apply_period(MYSQL *conn);
void show_apply_period(MYSQL *conn);
void save_apply_period(MYSQL *conn);
void delete_apply_period(MYSQL *conn);
int is_apply_period_open(MYSQL *conn);

#endif