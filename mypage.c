#include "mypage.h"
#include <stdio.h>
#include <string.h>


// ─────────────────────────────────────────────
// 헬퍼① — 내 게시글 확인/수정
// ─────────────────────────────────────────────
static void my_posts_menu(MYSQL *conn, const char *logged_id) {
  int choice;
  while (1) {
    printf("\n=== 내 게시글 확인/수정/삭제 ===\n");
    get_my_posts(conn, logged_id);
    
    printf("\n1. 수정  2. 삭제  0. 뒤로\n입력: ");
    if (scanf("%d", &choice) != 1) {
      while (getchar() != '\n');
      continue;
    }
    
    if (choice == 0) {
      return;
    } else if (choice == 1) {
      int post_id;
      printf("수정할 게시글 ID 입력 (0: 취소): ");
      if (scanf("%d", &post_id) != 1 || post_id == 0) {
        while (getchar() != '\n');
        continue;
      }
      
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
    } else if (choice == 2) {
      int post_id;
      printf("삭제할 게시글 ID 입력 (0: 취소): ");
      if (scanf("%d", &post_id) != 1 || post_id == 0) {
        while (getchar() != '\n');
        continue;
      }
      
      char confirm;
      printf("정말 삭제하시겠습니까? (y/n): ");
      scanf(" %c", &confirm);
      if (confirm == 'y' || confirm == 'Y') {
        if (delete_post(conn, post_id, logged_id)) {
          printf("✅ 삭제 완료!\n");
        } else {
          printf("❌ 삭제 실패 (본인 글이 아니거나 존재하지 않는 ID)\n");
        }
      } else {
        printf("삭제 취소되었습니다.\n");
      }
    } else {
      printf("잘못된 입력입니다.\n");
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
      char day[10], start[10], end_t[10], subject[100];
      printf("요일 (월/화/수/목/금/토/일): ");
      scanf("%9s", day);
      printf("시작시간 (예: 09:00): ");
      scanf("%9s", start);
      printf("종료시간 (예: 10:30): ");
      scanf("%9s", end_t);
      printf("과목명: ");
      scanf("%99s", subject);
      if (add_schedule(conn, logged_id, day, start, end_t, subject, "")) {
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

void viewMyMessages(MYSQL *conn, const char *logged_id) {
  printf("\n=== 📬 메시지함 ===\n");
  
  char query[512];
  sprintf(query, 
      "SELECT m.message_id, m.contented_at, m.is_read, m.sent_at "
      "FROM messages m JOIN users u ON m.receiver_idx = u.user_idx "
      "WHERE u.id = '%s' "
      "ORDER BY m.sent_at DESC", 
      logged_id
  );
  
  if (mysql_query(conn, query)) {
      fprintf(stderr, "메시지 로딩 실패: %s\n", mysql_error(conn));
      return;
  }
  
  MYSQL_RES *res = mysql_store_result(conn);
  if (res == NULL) {
      fprintf(stderr, "결과셋 로딩 실패\n");
      return;
  }
  
  int count = mysql_num_rows(res);
  if (count == 0) {
      printf("메시지함이 비어 있습니다.\n");
      mysql_free_result(res);
      return;
  }
  
  printf("%-8s %-50s %-10s %-20s\n", "메시지ID", "내용", "읽음상태", "수신일시");
  printf("----------------------------------------------------------------------------------------\n");
  
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res))) {
      const char *is_read_str = (atoi(row[2]) == 1) ? "읽음" : "안읽음";
      printf("%-8s %-50s %-10s %-20s\n", 
             row[0], row[1], is_read_str, row[3]);
  }
  mysql_free_result(res);
  
  // 조회 완료 시 읽음 상태로 일괄 업데이트
  sprintf(query, "UPDATE messages SET is_read = 1 "
                 "WHERE receiver_idx = (SELECT user_idx FROM users WHERE id = '%s') AND is_read = 0", logged_id);
  if (mysql_query(conn, query)) {
      fprintf(stderr, "읽음 상태 업데이트 실패: %s\n", mysql_error(conn));
  } else {
      printf("\n✅ 모든 새로운 메시지를 읽음 상태로 표시했습니다.\n");
  }
  
  printf("\n아무 키나 누르고 Enter를 입력하면 마이페이지로 돌아갑니다: ");
  char dummy[50];
  scanf("%49s", dummy);
}

// ─────────────────────────────────────────────
// 동아리장 전용 관리 메뉴
// ─────────────────────────────────────────────
static void club_owner_menu(MYSQL *conn, const char *logged_id, int club_id) {
  int choice;
  while (1) {
    printf("\n=== 동아리 관리 (동아리장 전용) ===\n");
    printf("1. 동아리원 강퇴\n");
    printf("2. 동아리 홍보글 작성\n");
    printf("3. 동아리장 위임\n");
    printf("4. 동아리 정보 수정\n");
    printf("0. 뒤로가기\n");
    printf("입력: ");
    if (scanf("%d", &choice) != 1) {
      while (getchar() != '\n');
      continue;
    }

    if (choice == 0) return;
    
    if (choice == 1) {
      char target_student_id[20];
      printf("강퇴할 동아리원의 학번 입력: ");
      scanf("%19s", target_student_id);
      kick_club_member(conn, club_id, logged_id, target_student_id);
    } 
    else if (choice == 2) {
      char title[512], content[4096];
      printf("홍보글 제목: ");
      while (getchar() != '\n');
      fgets(title, sizeof(title), stdin);
      title[strcspn(title, "\n")] = '\0';
      
      printf("홍보글 내용: ");
      fgets(content, sizeof(content), stdin);
      content[strcspn(content, "\n")] = '\0';
      
      create_promotion_post(conn, club_id, logged_id, title, content);
    }
    else if (choice == 3) {
      char target_student_id[20];
      printf("동아리장 권한을 위임할 동아리원의 학번 입력: ");
      scanf("%19s", target_student_id);
      if (transfer_club_ownership(conn, club_id, logged_id, target_student_id)) {
        return; // 성공 시 권한을 잃으므로 메뉴 종료
      }
    }
    else if (choice == 4) {
      char new_name[200], new_desc[1000];
      printf("새 동아리 이름: ");
      while (getchar() != '\n');
      fgets(new_name, sizeof(new_name), stdin);
      new_name[strcspn(new_name, "\n")] = '\0';
      
      printf("새 동아리 소개글: ");
      fgets(new_desc, sizeof(new_desc), stdin);
      new_desc[strcspn(new_desc, "\n")] = '\0';
      
      update_club_info(conn, club_id, logged_id, new_name, new_desc);
    }
    else {
      printf("잘못된 입력입니다.\n");
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
    printf("3. 메시지함\n");
    printf("4. 시간표 확인/수정\n");
    printf("5. 동아리 관리 (동아리장 전용)\n");
    printf("0. 뒤로\n");
    printf("============================\n");
    printf("입력: ");
    scanf("%d", &choice);

    switch (choice) {
      case 1: my_posts_menu(conn, logged_id);    break;
      case 2: my_comments_menu(conn, logged_id); break;
      case 3: {
        viewMyMessages(conn, logged_id);
        break;
      }
      case 4: my_schedule_menu(conn, logged_id); break;
      case 5: {
        int club_id = get_user_owned_club(conn, logged_id);
        if (club_id > 0) {
          club_owner_menu(conn, logged_id, club_id);
        } else {
          printf("[안내] 귀하는 승인된 동아리의 동아리장이 아닙니다.\n");
        }
        break;
      }
      case 0: return;
      default: printf("잘못된 입력입니다.\n");
    }
  }
}
