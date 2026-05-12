#ifndef BOARD_H
#define BOARD_H

#include "db.h"

// 동아리 홍보 게시판 (카테고리 ID 1)
void promo_board(MYSQL *conn);

// 전공 동아리 게시판 (카테고리 ID 2)
void major_club_board(MYSQL *conn);

#endif // BOARD_H
