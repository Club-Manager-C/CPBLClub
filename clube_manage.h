#ifndef CLUB_MANAGE_H
#define CLUB_MANAGE_H
#include <mysql.h>

void club_manage_menu(MYSQL *conn);

void view_pending_clubs(MYSQL *conn);

void approve_club(MYSQL *conn);

void reject_club(MYSQL *conn);

void delete_club(MYSQL *conn);

#endif