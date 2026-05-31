#include "mypage.h"
#include "board.h"
#include "filter.h"
#include <stdio.h>
#include <string.h>
#include "major_info.h"

// ─────────────────────────────────────────────
// 헬퍼① — 내 게시글 확인/수정
// ─────────────────────────────────────────────
static void my_posts_menu(MYSQL *conn, const char *logged_id) {
  int choice;
  while (1) {
    if (is_currently_suspended(conn, logged_id)) return;
    printf("\n=== 내 게시글 확인/수정/삭제 ===\n");
    get_my_posts(conn, logged_id);

    printf("\n1. 수정  2. 삭제  0. 뒤로\n입력: ");
    if (scanf("%d", &choice) != 1) {
      while (getchar() != '\n')
        ;
      continue;
    }

    if (choice == 0) {
      return;
    } else if (choice == 1) {
      int post_id;
      printf("수정할 게시글 ID 입력 (0: 취소): ");
      if (scanf("%d", &post_id) != 1 || post_id == 0) {
        while (getchar() != '\n')
          ;
        continue;
      }

      char new_content[500];
      printf("새 내용 입력: ");
      while (getchar() != '\n')
        ; // 버퍼 정리
      fgets(new_content, sizeof(new_content), stdin);
      new_content[strcspn(new_content, "\n")] = '\0';

      // 비속어 필터링 검사
      if (contains_slang(new_content)) {
        printf("❌ 부적절한 단어(욕설 등)가 포함되어 있어 글을 수정할 수 "
               "없습니다.\n");
        increment_bad_word_count(conn, logged_id);
        continue;
      }

      if (update_post(conn, post_id, logged_id, new_content)) {
        printf("✅ 수정 완료!\n");
      } else {
        printf("❌ 수정 실패 (본인 글이 아니거나 존재하지 않는 ID)\n");
      }
    } else if (choice == 2) {
      int post_id;
      printf("삭제할 게시글 ID 입력 (0: 취소): ");
      if (scanf("%d", &post_id) != 1 || post_id == 0) {
        while (getchar() != '\n')
          ;
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
    if (is_currently_suspended(conn, logged_id)) return;
    printf("\n=== 내 댓글 확인/수정 ===\n");
    get_my_comments(conn, logged_id);
    printf("\n수정할 댓글 ID 입력 (0: 뒤로): ");
    scanf("%d", &comment_id);
    if (comment_id == 0)
      return;

    char new_content[500];
    printf("새 내용 입력: ");
    while (getchar() != '\n')
      ;
    fgets(new_content, sizeof(new_content), stdin);
    new_content[strcspn(new_content, "\n")] = '\0';

    // 비속어 필터링 검사
    if (contains_slang(new_content)) {
      printf("❌ 부적절한 단어(욕설 등)가 포함되어 있어 댓글을 수정할 수 "
             "없습니다.\n");
      increment_bad_word_count(conn, logged_id);
      continue;
    }

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
    if (is_currently_suspended(conn, logged_id)) return;
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

  char query[1024];
  sprintf(query,
          "SELECT m.message_id, m.contented_at, m.is_read, m.sent_at, m.msg_type, m.sender_info "
          "FROM messages m JOIN users u ON m.receiver_idx = u.user_idx "
          "WHERE u.id = '%s' "
          "ORDER BY m.sent_at DESC",
          logged_id);

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

  printf("----------------------------------------------------------------------------------------\n");
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res))) {
    const char *is_read_str = (atoi(row[2]) == 1) ? "[읽음]" : "[안읽음]";
    int msg_type = row[4] ? atoi(row[4]) : 1;
    const char *sender_info = row[5] ? row[5] : "System";
    
    char prefix[20];
    if (msg_type == 0) {
        strcpy(prefix, "[공지]");
    } else {
        strcpy(prefix, "[쪽지]");
    }
    
    printf("%s %-10s 발신: %-15s | 내용: %s | 수신일: %s\n", is_read_str, prefix, sender_info, row[1], row[3]);
  }
  mysql_free_result(res);

  // 조회 완료 시 읽음 상태로 일괄 업데이트
  sprintf(query,
          "UPDATE messages SET is_read = 1 "
          "WHERE receiver_idx = (SELECT user_idx FROM users WHERE id = '%s') "
          "AND is_read = 0",
          logged_id);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "읽음 상태 업데이트 실패: %s\n", mysql_error(conn));
  } else {
    printf("\n✅ 모든 새로운 메시지를 읽음 상태로 표시했습니다.\n");
  }

  printf("\n아무 키나 누르고 Enter를 입력하면 마이페이지로 돌아갑니다: ");
  char dummy[50];
  scanf("%49s", dummy);
}

void send_dm_menu(MYSQL *conn, const char *logged_id) {
    char receiver_id[100];
    char content[1024];

    printf("\n--- 쪽지 보내기 ---\n");
    printf("받는 사람 ID: ");
    scanf("%99s", receiver_id);
    getchar(); // 버퍼 비우기

    printf("쪽지 내용 입력 (최대 1000자): ");
    fgets(content, sizeof(content), stdin);
    content[strcspn(content, "\r\n")] = '\0'; // 개행 문자 제거

    if (strlen(content) == 0) {
        printf("⚠️ 쪽지 내용을 입력해주세요.\n");
        return;
    }

    send_direct_message(conn, logged_id, receiver_id, content);
}

void send_announcement_menu(MYSQL *conn, const char *logged_id) {
    int club_id;
    char content[1024];

    printf("\n--- 단체 공지 발송 (동아리장 전용) ---\n");

    char query[512];
    char esc_logged_id[100];
    mysql_real_escape_string(conn, esc_logged_id, logged_id, strlen(logged_id));

    // 사용자가 방장인 동아리 목록 출력
    sprintf(query, 
            "SELECT c.club_id, c.club_name FROM clubs c "
            "JOIN clubmembers cm ON c.club_id = cm.club_id "
            "JOIN users u ON cm.user_idx = u.user_idx "
            "WHERE u.id = '%s' AND cm.role = 'Leader'", esc_logged_id);
    
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *res = mysql_store_result(conn);
        if (res && mysql_num_rows(res) > 0) {
            printf("[내 관리 동아리 목록]\n");
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                printf(" - ID: %s | 동아리명: %s\n", row[0], row[1]);
            }
            mysql_free_result(res);
            printf("--------------------------------------\n");
        } else {
            if (res) mysql_free_result(res);
            printf("⚠️ 현재 동아리장으로 관리 중인 동아리가 없습니다.\n");
            return;
        }
    }

    printf("공지를 발송할 동아리 ID(숫자): ");
    if (scanf("%d", &club_id) != 1) {
        printf("⚠️ 잘못된 입력입니다. 동아리 ID는 숫자로 입력해주세요.\n");
        while (getchar() != '\n'); // 버퍼 비우기
        return;
    }
    getchar(); // 버퍼 비우기

    printf("공지 내용 입력 (최대 1000자): ");
    fgets(content, sizeof(content), stdin);
    content[strcspn(content, "\r\n")] = '\0';

    if (strlen(content) == 0) {
        printf("⚠️ 공지 내용을 입력해주세요.\n");
        return;
    }

    send_club_announcement(conn, club_id, logged_id, content);
}

void manage_join_requests(MYSQL *conn, const char *logged_id) {
    char query[1024];
    char esc_logged_id[100];
    mysql_real_escape_string(conn, esc_logged_id, logged_id, strlen(logged_id));

    while (1) {
        if (is_currently_suspended(conn, logged_id)) return;
        printf("\n=== 가입 신청 관리 (동아리장 전용) ===\n");
        
        sprintf(query, 
            "SELECT j.request_id, u.student_id, u.name, u.college_code, u.major_code, j.introduction, c.club_name "
            "FROM joinrequests j "
            "JOIN users u ON j.user_idx = u.user_idx "
            "JOIN clubs c ON j.club_id = c.club_id "
            "JOIN clubmembers cm ON c.club_id = cm.club_id "
            "WHERE cm.user_idx = (SELECT user_idx FROM users WHERE id='%s') "
            "AND cm.role = 'Leader' AND j.status = '대기'", esc_logged_id);

        if (mysql_query(conn, query)) {
            fprintf(stderr, "대기 목록 조회 실패: %s\n", mysql_error(conn));
            return;
        }

        MYSQL_RES *res = mysql_store_result(conn);
        if (res == NULL) {
            fprintf(stderr, "결과셋 로드 실패\n");
            return;
        }

        int row_count = mysql_num_rows(res);
        if (row_count == 0) {
            printf("대기 중인 가입 신청이 없습니다.\n");
            mysql_free_result(res);
            return;
        }

        printf("%-8s %-15s %-15s %-20s %-20s\n", "신청ID", "학번", "이름", "전공", "동아리명");
        printf("--------------------------------------------------------------------------------\n");

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            int u_col = atoi(row[3]);
            int u_maj = atoi(row[4]);
            printf("%-8s %-15s %-15s %-20s %-20s\n", 
                   row[0], row[1], row[2], get_major_name(u_col, u_maj), row[6]);
            printf("   자기소개: %s\n", row[5]);
            printf("--------------------------------------------------------------------------------\n");
        }
        mysql_free_result(res);

        int target_req_id;
        printf("\n승인/거절 처리할 신청 ID (0: 뒤로가기): ");
        if (scanf("%d", &target_req_id) != 1) {
            while (getchar() != '\n');
            printf("잘못된 입력입니다.\n");
            continue;
        }
        if (target_req_id == 0) return;

        sprintf(query,
            "SELECT j.club_id, u.id, u.name, c.club_name "
            "FROM joinrequests j "
            "JOIN users u ON j.user_idx = u.user_idx "
            "JOIN clubs c ON j.club_id = c.club_id "
            "JOIN clubmembers cm ON c.club_id = cm.club_id "
            "WHERE j.request_id = %d AND j.status = '대기' "
            "AND cm.user_idx = (SELECT user_idx FROM users WHERE id='%s') AND cm.role = 'Leader'",
            target_req_id, esc_logged_id);
            
        if (mysql_query(conn, query)) {
            fprintf(stderr, "검증 쿼리 실패: %s\n", mysql_error(conn));
            continue;
        }

        MYSQL_RES *check_res = mysql_store_result(conn);
        if (check_res == NULL || mysql_num_rows(check_res) == 0) {
            printf("해당 신청 내역이 존재하지 않거나 권한이 없습니다.\n");
            if (check_res) mysql_free_result(check_res);
            continue;
        }

        MYSQL_ROW check_row = mysql_fetch_row(check_res);
        int club_id = atoi(check_row[0]);
        char applicant_id[50]; strcpy(applicant_id, check_row[1]);
        char applicant_name[50]; strcpy(applicant_name, check_row[2]);
        char club_name[100]; strcpy(club_name, check_row[3]);
        mysql_free_result(check_res);

        int action;
        printf("\n신청자: %s\n", applicant_name);
        printf("1. 승인  2. 거절  0. 취소\n입력: ");
        if (scanf("%d", &action) != 1) {
            while(getchar() != '\n');
            printf("잘못된 입력입니다.\n");
            continue;
        }

        if (action == 1) {
            sprintf(query, "UPDATE joinrequests SET status = '승인' WHERE request_id = %d", target_req_id);
            mysql_query(conn, query);

            sprintf(query, "INSERT IGNORE INTO clubmembers (club_id, user_idx, role) VALUES (%d, (SELECT user_idx FROM users WHERE id = '%s'), 'Member')", club_id, applicant_id);
            mysql_query(conn, query);

            char msg_content[500];
            sprintf(msg_content, "축하합니다! [%s] 동아리 가입이 승인되었습니다.", club_name);
            insert_message(conn, applicant_id, msg_content);

            printf("✅ [%s] 님의 가입 신청이 승인되었습니다.\n", applicant_name);
        } else if (action == 2) {
            sprintf(query, "UPDATE joinrequests SET status = '거절' WHERE request_id = %d", target_req_id);
            mysql_query(conn, query);

            char msg_content[500];
            sprintf(msg_content, "[%s] 동아리 가입 신청이 거절되었습니다.", club_name);
            insert_message(conn, applicant_id, msg_content);

            printf("❌ [%s] 님의 가입 신청이 거절되었습니다.\n", applicant_name);
        } else {
            printf("취소되었습니다.\n");
        }
    }
}

void club_leader_menu(MYSQL *conn, const char *logged_id) {
    int choice;
    while (1) {
        if (is_currently_suspended(conn, logged_id)) return;
        printf("\n============================\n");
        printf("  동아리장 관리 메뉴\n");
        printf("============================\n");
        printf("1. 단체 공지 발송\n");
        printf("2. 가입 신청 관리\n");
        printf("0. 뒤로\n");
        printf("============================\n");
        printf("입력: ");
        if (scanf("%d", &choice) != 1) {
            while(getchar() != '\n');
            continue;
        }

        switch (choice) {
            case 1: send_announcement_menu(conn, logged_id); break;
            case 2: manage_join_requests(conn, logged_id); break;
            case 0: return;
            default: printf("잘못된 입력입니다.\n");
        }
    }
}

void edit_my_info(MYSQL *conn, const char *logged_id) {
    char esc_id[100];
    mysql_real_escape_string(conn, esc_id, logged_id, strlen(logged_id));
    
    char pw[50];
    printf("\n비밀번호를 입력하세요: ");
    scanf("%s", pw);
    
    if (check_login(conn, logged_id, pw) != 1) {
        printf("비밀번호가 일치하지 않습니다.\n");
        return;
    }
    
    char new_nickname[50], new_pw[50];
    printf("새 닉네임: ");
    scanf("%s", new_nickname);
    
    char esc_nick[100];
    mysql_real_escape_string(conn, esc_nick, new_nickname, strlen(new_nickname));
    
    char query[512];
    sprintf(query, "SELECT 1 FROM users WHERE nickname='%s' AND id != '%s'", esc_nick, esc_id);
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *res = mysql_store_result(conn);
        if (res && mysql_num_rows(res) > 0) {
            printf("이미 존재하는 닉네임입니다.\n");
            mysql_free_result(res);
            return;
        }
        if (res) mysql_free_result(res);
    }
    
    printf("새 비밀번호: ");
    scanf("%s", new_pw);
    
    char esc_pw[100];
    mysql_real_escape_string(conn, esc_pw, new_pw, strlen(new_pw));
    
    sprintf(query, "UPDATE users SET nickname='%s', pw='%s' WHERE id='%s'", esc_nick, esc_pw, esc_id);
    if (mysql_query(conn, query)) {
        printf("정보 수정 실패: %s\n", mysql_error(conn));
    } else {
        printf("✅ 정보가 성공적으로 수정되었습니다.\n");
    }
}

void delete_my_account(MYSQL *conn, const char *logged_id) {
    char esc_id[100];
    mysql_real_escape_string(conn, esc_id, logged_id, strlen(logged_id));
    
    char query[512];
    sprintf(query, "SELECT count(*) FROM clubmembers WHERE user_idx = (SELECT user_idx FROM users WHERE id='%s') AND role = 'Leader'", esc_id);
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *res = mysql_store_result(conn);
        if (res) {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row && atoi(row[0]) > 0) {
                printf("\n❌ 동아리장(OWNER) 권한을 가진 동아리가 있어 탈퇴할 수 없습니다. 위임이나 폐쇄를 먼저 진행해주세요.\n");
                mysql_free_result(res);
                return;
            }
            mysql_free_result(res);
        }
    }
    
    char pw[50];
    printf("\n탈퇴하시려면 비밀번호를 입력하세요: ");
    scanf("%s", pw);
    if (check_login(conn, logged_id, pw) != 1) {
        printf("비밀번호가 일치하지 않습니다.\n");
        return;
    }
    
    sprintf(query, "DELETE FROM users WHERE id='%s'", esc_id);
    if (mysql_query(conn, query)) {
        printf("탈퇴 처리 실패: %s\n", mysql_error(conn));
    } else {
        printf("\n✅ 회원 탈퇴가 완료되었습니다. 이용해 주셔서 감사합니다.\n");
        // Force logout by exiting the my_page loop (the caller should handle it)
        // Since my_page doesn't have a way to force logout to main easily without global variable,
        // we'll just return and let the user log out manually or we can exit the program.
        printf("프로그램을 종료합니다.\n");
        exit(0);
    }
}

// ─────────────────────────────────────────────
// 마이페이지 메인
// ─────────────────────────────────────────────
void my_page(MYSQL *conn, const char *logged_id) {
  int choice;
  while (1) {
    if (is_currently_suspended(conn, logged_id)) return;
    display_my_profile(conn, logged_id);

    printf("\n============================\n");
    printf("  마이페이지 메뉴\n");
    printf("============================\n");
    printf("1. 내 게시글 확인/수정\n");
    printf("2. 내 댓글 확인/수정\n");
    printf("3. 메시지함\n");
    printf("4. 쪽지 보내기\n");
    printf("5. 시간표 확인/수정\n");
    printf("6. 동아리장 관리 메뉴 (동아리장 전용)\n");
    printf("7. 내 정보 수정\n");
    printf("8. 회원 탈퇴\n");
    printf("0. 뒤로\n");
    printf("============================\n");
    printf("입력: ");
    if (scanf("%d", &choice) != 1) {
        while (getchar() != '\n');
        continue;
    }

    switch (choice) {
      case 1: my_posts_menu(conn, logged_id);    break;
      case 2: my_comments_menu(conn, logged_id); break;
      case 3: viewMyMessages(conn, logged_id);   break;
      case 4: send_dm_menu(conn, logged_id);     break;
      case 5: my_schedule_menu(conn, logged_id); break;
      case 6: club_leader_menu(conn, logged_id); break;
      case 7: edit_my_info(conn, logged_id);     break;
      case 8: delete_my_account(conn, logged_id); break;
      case 0: return;
      default: printf("잘못된 입력입니다.\n");
    }
  }
}
