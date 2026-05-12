#include "board.h"
#include <stdio.h>

// ─────────────────────────────────────────────
// 동아리 홍보 게시판 (카테고리 ID 1)
// ─────────────────────────────────────────────
void promo_board(MYSQL *conn) {
  printf("\n=== 동아리 홍보 게시판 ===\n");
  get_posts_by_category(conn, 1);
}

// ─────────────────────────────────────────────
// 전공 동아리 게시판 (카테고리 ID 2)
// ─────────────────────────────────────────────
void major_club_board(MYSQL *conn) {
  printf("\n=== 전공 동아리 게시판 ===\n");
  get_posts_by_category(conn, 2);
}
