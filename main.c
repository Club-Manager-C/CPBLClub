#include "db.h"
#include "auth.h"
#include "board.h"
#include "mypage.h"
#include <stdio.h>
#include <stdlib.h>

// ─────────────────────────────────────────────
// 동아리장 신청 (스텁)
// ─────────────────────────────────────────────
void apply_club_leader() {
  printf("\n=== 동아리장 신청 ===\n");
  printf("(준비 중입니다)\n");
}

// ─────────────────────────────────────────────
// 메인 화면 (로그인 후 진입)
// ─────────────────────────────────────────────
void home_screen(MYSQL *conn, const char *logged_id) {
  int choice;

  while (1) {
    printf("\n============================\n");
    printf("  메인 메뉴\n");
    printf("============================\n");
    printf("1. 동아리 홍보 게시판\n");
    printf("2. 전공 동아리 게시판\n");
    printf("3. 마이페이지\n");
    printf("4. 동아리장 신청\n");
    printf("0. 로그아웃\n");
    printf("============================\n");
    printf("입력: ");
    scanf("%d", &choice);

    switch (choice) {
      case 1: promo_board(conn);           break;
      case 2: major_club_board(conn);      break;
      case 3: my_page(conn, logged_id);    break;
      case 4: apply_club_leader();         break;
      case 0:
        printf("로그아웃 합니다.\n");
        return;
      default:
        printf("잘못된 입력입니다.\n");
    }
  }
}

// ─────────────────────────────────────────────
// main
// ─────────────────────────────────────────────
int main() {
  MYSQL *conn;
  init_db(&conn);

  int choice;
  char logged_id[50];

  while (1) {
    printf("\n=== 시작 화면 ===\n");
    printf("1. 로그인\n");
    printf("2. 회원가입\n");
    printf("3. 종료\n");
    printf("입력: ");
    scanf("%d", &choice);

    if (choice == 1) {
      if (login_screen(conn, logged_id)) {
        home_screen(conn, logged_id);
      }
    } else if (choice == 2) {
      register_screen(conn);
    } else if (choice == 3) {
      printf("프로그램을 종료합니다.\n");
      break;
    } else {
      printf("잘못된 입력입니다.\n");
    }
  }

  close_db(conn);
  return 0;
}
