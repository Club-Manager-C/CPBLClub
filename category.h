#ifndef CATEGORY_H
#define CATEGORY_H

#include <windows.h>
#include <mysql.h>

void category_menu(MYSQL *conn);
void view_categories(MYSQL *conn);
int add_category(MYSQL *conn);
int update_category(MYSQL *conn);
int delete_category(MYSQL *conn);

#endif // DB_H