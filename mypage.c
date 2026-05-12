#include "mypage.h"
#include <stdio.h>
#include <string.h>

// ─────────────────────────────────────────────
// 헬퍼① — 내 게시글 확인/수정
// ─────────────────────────────────────────────
static void my_posts_menu(MYSQL *conn, const char *logged_id) {
  int post_id;
  while (1) {
    printf("\n=== 내 게시글 확인/수정 ===\n");
    get_my_posts(conn, logged_id);
    printf("\n수정할 게시글 ID 입력 (0: 뒤로): ");
    scanf("%d", &post_id);
    if (post_id == 0) return;

    char new_content[500];
    printf("새 내용 입력: ");
    while (getchar() != '\n'); // 버퍼 정리
    fgets(new_content, sizeof(new_content), stdin);
    new_content[strcspn(new_content, "\n")] = '\0';

    if (update_post(conn, post_id, logged_id, new_content)) {
      printf("✅ 수정 완료!\n");
    } else {
      printf("❌ 수정 실패 (본인 글이 아니거나 존재하지 않는 ID)\n");
    }
  }
}

// ─────────────────────────────────────────────
// 헬퍼② — 내 댓글 확인/수정
// ─────────────────────────────────────────────
static void my_comments_menu(MYSQL *conn, const char *logged_id) {
  int comment_id;
  while (1) {
    printf("\n=== 내 댓글 확인/수정 ===\n");
    get_my_comments(conn, logged_id);
    printf("\n수정할 댓글 ID 입력 (0: 뒤로): ");
    scanf("%d", &comment_id);
    if (comment_id == 0) return;

    char new_content[500];
    printf("새 내용 입력: ");
    while (getchar() != '\n');
    fgets(new_content, sizeof(new_content), stdin);
    new_content[strcspn(new_content, "\n")] = '\0';

    if (update_comment(conn, comment_id, logged_id, new_content)) {
      printf("✅ 수정 완료!\n");
    } else {
      printf("❌ 수정 실패 (본인 댓글이 아니거나 존재하지 않는 ID)\n");
    }
  }
}

// ─────────────────────────────────────────────
// 헬퍼③ — 시간표 확인/수정
// ─────────────────────────────────────────────
static void my_schedule_menu(MYSQL *conn, const char *logged_id) {
  int choice;
  while (1) {
    printf("\n=== 시간표 확인/수정 ===\n");
    get_my_schedule(conn, logged_id);
    printf("\n1. 추가  2. 삭제  0. 뒤로\n입력: ");
    scanf("%d", &choice);

    if (choice == 0) {
      return;
    } else if (choice == 1) {
      char day[10], start[10], end_t[10], subject[100], location[100];
      printf("요일 (월/화/수/목/금/토/일): ");
      scanf("%9s", day);
      printf("시작시간 (예: 09:00): ");
      scanf("%9s", start);
      printf("종료시간 (예: 10:30): ");
      scanf("%9s", end_t);
      printf("과목명: ");
      scanf("%99s", subject);
      printf("강의실: ");
      scanf("%99s", location);
      if (add_schedule(conn, logged_id, day, start, end_t, subject, location)) {
        printf("✅ 추가 완료!\n");
      } else {
        printf("❌ 추가 실패\n");
      }
    } else if (choice == 2) {
      int sid;
      printf("삭제할 시간표 ID 입력: ");
      scanf("%d", &sid);
      if (delete_schedule(conn, sid, logged_id)) {
        printf("✅ 삭제 완료!\n");
      } else {
        printf("❌ 삭제 실패 (본인 항목이 아니거나 존재하지 않는 ID)\n");
      }
    } else {
      printf("잘못된 입력\n");
    }
  }
}

// ─────────────────────────────────────────────
// 마이페이지 메인
// ─────────────────────────────────────────────
void my_page(MYSQL *conn, const char *logged_id) {
  int choice;
  while (1) {
    printf("\n============================\n");
    printf("  마이페이지 (%s)\n", logged_id);
    printf("============================\n");
    printf("1. 내 게시글 확인/수정\n");
    printf("2. 내 댓글 확인/수정\n");
    printf("3. 시간표 확인/수정\n");
    printf("0. 뒤로\n");
    printf("============================\n");
    printf("입력: ");
    scanf("%d", &choice);

    switch (choice) {
      case 1: my_posts_menu(conn, logged_id);    break;
      case 2: my_comments_menu(conn, logged_id); break;
      case 3: my_schedule_menu(conn, logged_id); break;
      case 0: return;
      default: printf("잘못된 입력입니다.\n");
    }
  }
}
