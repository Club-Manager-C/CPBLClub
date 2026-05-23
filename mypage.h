#ifndef MYPAGE_H
#define MYPAGE_H

#include "db.h"

// 마이페이지 메인 메뉴
void my_page(MYSQL *conn, const char *logged_id);
void viewMyMessages(MYSQL *conn, const char *logged_id);

#endif // MYPAGE_H
