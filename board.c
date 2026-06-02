#include "board.h"
#include "filter.h"
#include <stdbool.h>
#include "major_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 전방 선언
void show_board_menu(MYSQL *conn, int category_id, const char *logged_id);
void write_post_menu(MYSQL *conn, int category_id, const char *logged_id);
void view_post_detail_menu(MYSQL *conn, int post_id, const char *logged_id);
int print_post_detail(MYSQL *conn, int post_id);
void get_post_comments_list(MYSQL *conn, int post_id, int *comment_ids_out,
                            int *comment_count_out);
void view_comment_detail_menu(MYSQL *conn, int comment_id,
                              const char *logged_id);
void search_board_menu(MYSQL *conn, int category_id, const char *logged_id);
int is_string_whitespace_only(const char *str);

// ─────────────────────────────────────────────
// 동아리 홍보 게시판 (카테고리 ID 1)
// ─────────────────────────────────────────────
void promo_board(MYSQL *conn, const char *logged_id) {
  show_board_menu(conn, 1, logged_id);
}

// ─────────────────────────────────────────────
// 전공 동아리 게시판 (카테고리 ID 2)
// ─────────────────────────────────────────────
void major_club_board(MYSQL *conn, const char *logged_id) {
  show_board_menu(conn, 2, logged_id);
}

// ─────────────────────────────────────────────
// 게시판 통합 메뉴 루프
// ─────────────────────────────────────────────
void show_board_menu(MYSQL *conn, int category_id, const char *logged_id) {
  char cat_name[100] = "게시판";
  char query[256];
  sprintf(query, "SELECT category_name FROM club_categories WHERE category_id = %d", category_id);
  if (mysql_query(conn, query) == 0) {
      MYSQL_RES *res = mysql_store_result(conn);
      if (res && mysql_num_rows(res) > 0) {
          MYSQL_ROW row = mysql_fetch_row(res);
          sprintf(cat_name, "%s 게시판", row[0]);
      }
      if (res) mysql_free_result(res);
  }

  int choice;
  while (1) {
    if (is_currently_suspended(conn, logged_id)) return;
    printf("\n");
    printf("        [ %s ]\n", cat_name);
    printf("\n");

    get_posts_by_category(conn, category_id);

    printf("\n\n\n");
    printf("------------------------------------------------------------\n");
    printf("  [1] 글 상세 보기   [2] 글 작성   [3] 검색   [0] 뒤로가기\n");
    printf("------------------------------------------------------------\n");
    printf("입력: ");
    if (scanf("%d", &choice) != 1) {
      while (getchar() != '\n')
        ; // 버퍼 비우기
      continue;
    }

    if (choice == 0) {
      break;
    } else if (choice == 1) {
      int post_id;
      printf("조회할 글 ID(번호) 입력: ");
      if (scanf("%d", &post_id) != 1) {
        while (getchar() != '\n')
          ;
        continue;
      }
      view_post_detail_menu(conn, post_id, logged_id);
    } else if (choice == 2) {
      write_post_menu(conn, category_id, logged_id);
    } else if (choice == 3) {
      search_board_menu(conn, category_id, logged_id);
    } else {
      printf("잘못된 입력입니다.\n");
    }
  }
}

int is_string_whitespace_only(const char *str) {
  if (str == NULL || strlen(str) == 0) return 1;
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] != ' ' && str[i] != '\t' && str[i] != '\r' && str[i] != '\n') {
      return 0; // 공백이 아닌 문자가 최소 하나 이상 포함됨
    }
  }
  return 1; // 모든 문자가 공백 문자
}

void search_board_menu(MYSQL *conn, int category_id, const char *logged_id) {
  char keyword[100];
  int choice;

  while (1) {
    if (is_currently_suspended(conn, logged_id)) return;

    printf("\n검색할 키워드를 입력하세요 (최대 30자): ");
    while (getchar() != '\n'); // 이전 입력 버퍼 비우기 (중요!)

    if (fgets(keyword, sizeof(keyword), stdin) == NULL) {
      printf("입력 오류가 발생했습니다.\n");
      return;
    }

    // 개행 문자 제거
    keyword[strcspn(keyword, "\n")] = '\0';

    // 30자 초과 시 자르기
    if (strlen(keyword) > 30) {
      keyword[30] = '\0';
    }

    // 공백 입력 예외 처리
    if (is_string_whitespace_only(keyword)) {
      printf("\n❌ 검색어에 공백만 입력할 수 없습니다.\n");
      return;
    }

    // DB 조회 및 결과에 따른 하위 분기 루프
    while (1) {
      if (is_currently_suspended(conn, logged_id)) return;

      int result_count = search_posts_by_keyword(conn, category_id, keyword);

      if (result_count > 0) {
        // Case A: 검색 결과가 있을 때
        printf("\n1. 게시글 상세 보기\n");
        printf("0. 뒤로 가기\n");
        printf("선택: ");
        if (scanf("%d", &choice) != 1) {
          while (getchar() != '\n'); // 버퍼 비우기
          printf("잘못된 입력입니다.\n");
          continue;
        }

        if (choice == 1) {
          int post_id;
          printf("조회할 글 ID(번호) 입력: ");
          if (scanf("%d", &post_id) != 1) {
            while (getchar() != '\n');
            printf("잘못된 번호입니다.\n");
            continue;
          }
          view_post_detail_menu(conn, post_id, logged_id);
          // 상세보기 이후 다시 루프를 돌며 검색 결과 및 메뉴를 계속 보여줌
        } else if (choice == 0) {
          return; // 검색 화면 종료 및 메인 메뉴로 복귀
        } else {
          printf("잘못된 메뉴 번호입니다.\n");
        }
      } else {
        // Case B: 검색 결과가 없을 때 (해당 검색어를 포함하는 게시글이 없습니다.)
        printf("\n❌ 해당 검색어를 포함하는 게시글이 없습니다.\n");
        printf("\n1. 다시 검색\n");
        printf("0. 뒤로 가기\n");
        printf("선택: ");
        if (scanf("%d", &choice) != 1) {
          while (getchar() != '\n'); // 버퍼 비우기
          printf("잘못된 입력입니다.\n");
          continue;
        }

        if (choice == 1) {
          break; // 안쪽 루프를 깨고 바깥쪽 검색어 입력 단계로 회귀
        } else if (choice == 0) {
          return; // 검색 화면 종료 및 메인 메뉴로 복귀
        } else {
          printf("잘못된 메뉴 번호입니다.\n");
        }
      }
    }
  }
}

// ─────────────────────────────────────────────
// 게시글 작성 화면
// ─────────────────────────────────────────────
void write_post_menu(MYSQL *conn, int category_id, const char *logged_id) {
  // 권한 검증: 현재 카테고리에 속한 동아리의 방장인지 확인하고 club_id 가져오기
  char auth_query[512];
  sprintf(auth_query, 
      "SELECT club_id FROM clubs WHERE leader_idx = (SELECT user_idx FROM users WHERE id = '%s') AND category_id = %d AND status = '승인'", 
      logged_id, category_id);
  
  int club_id = 0;
  if (mysql_query(conn, auth_query) == 0) {
      MYSQL_RES *res = mysql_store_result(conn);
      if (!res || mysql_num_rows(res) == 0) {
          if (res) mysql_free_result(res);
          printf("\n❌ 작성 권한이 없습니다. 해당 카테고리에 속한 동아리의 방장만 게시글을 작성할 수 있습니다.\n");
          return;
      }
      MYSQL_ROW row = mysql_fetch_row(res);
      club_id = atoi(row[0]);
      mysql_free_result(res);
  } else {
      printf("\n❌ 권한 확인 중 오류가 발생했습니다.\n");
      return;
  }

  char title[200];
  char content[1000];

  printf("\n");
  printf("              [ 새 글 작성 ]\n");
  printf("\n");
  printf("제목 입력: ");
  while (getchar() != '\n')
    ; // 버퍼 비우기
  fgets(title, sizeof(title), stdin);
  title[strcspn(title, "\n")] = '\0';

  printf("내용 입력: ");
  fgets(content, sizeof(content), stdin);
  content[strcspn(content, "\n")] = '\0';

  // 비속어 필터링 검사
  if (contains_slang(title) || contains_slang(content)) {
    printf("❌ 부적절한 단어(욕설 등)가 포함되어 있어 글을 등록할 수 없습니다.\n");
    increment_bad_word_count(conn, logged_id);
    return;
  }

  if (insert_post(conn, club_id, logged_id, category_id, title, content)) {
    printf("\n");
    printf("✅ 글이 성공적으로 등록되었습니다!\n");
    printf("\n");
    wait_enter_and_clear("Enter 키를 눌러 게시글 목록으로 돌아갑니다...");
  } else {
    printf("❌ 글 등록에 실패했습니다.\n");
    wait_enter_and_clear("Enter 키를 눌러 게시글 목록으로 돌아갑니다...");
  }
}

// ─────────────────────────────────────────────
// 게시글 본문 상세 정보 출력
// ─────────────────────────────────────────────
int print_post_detail(MYSQL *conn, int post_id) {
  char query[512];
  sprintf(query,
          "SELECT p.title, p.content, u.name, u.college_code, u.major_code, c.club_name, c.college_code, c.major_code, p.created_at, p.like_count "
          "FROM posts p "
          "JOIN users u ON p.user_idx = u.user_idx "
          "JOIN clubs c ON p.club_id = c.club_id "
          "WHERE p.post_id = %d",
          post_id);
  if (mysql_query(conn, query)) {
    printf("게시글 로딩 에러: %s\n", mysql_error(conn));
    return 0;
  }
  MYSQL_RES *res = mysql_store_result(conn);
  if (res == NULL || mysql_num_rows(res) == 0) {
    if (res)
      mysql_free_result(res);
    return 0;
  }
  MYSQL_ROW row = mysql_fetch_row(res);
  
  char title_str[150];
  if (row[6] && row[7]) {
      int c_col = atoi(row[6]);
      int c_maj = atoi(row[7]);
      if (c_col > 0) {
          sprintf(title_str, "[%s-%s] %s", get_college_name(c_col), get_major_name(c_col, c_maj), row[0]);
      } else {
          strcpy(title_str, row[0]);
      }
  } else {
      strcpy(title_str, row[0]);
  }
  
  int u_col = atoi(row[3]);
  int u_maj = atoi(row[4]);

  printf("\n==================================================\n");
  printf(" 제목: %s\n", title_str);
  printf(" [작성자: %s (%s) | 등록 동아리: %s]\n", row[2], get_major_name(u_col, u_maj), row[5]);
  printf(" 작성일: %s | 좋아요: %s\n", row[8], row[9]);
  printf("--------------------------------------------------\n");
  printf(" %s\n", row[1]);
  printf("==================================================\n");
  mysql_free_result(res);
  return 1;
}

// ─────────────────────────────────────────────
// 게시글 상세 메뉴 루프
// ─────────────────────────────────────────────
void view_post_detail_menu(MYSQL *conn, int post_id, const char *logged_id) {
  int choice;

  // 게시글이 속한 club_id 조회
  int club_id = 0;
  char query[256];
  sprintf(query, "SELECT club_id FROM posts WHERE post_id = %d", post_id);
  if (mysql_query(conn, query) == 0) {
      MYSQL_RES *res = mysql_store_result(conn);
      if (res && mysql_num_rows(res) > 0) {
          MYSQL_ROW row = mysql_fetch_row(res);
          club_id = atoi(row[0]);
      }
      if (res) mysql_free_result(res);
  }

  while (1) {
    if (is_currently_suspended(conn, logged_id)) return;
    if (!print_post_detail(conn, post_id)) {
      printf("존재하지 않는 게시글입니다.\n");
      return;
    }

    printf("1) 댓글 확인\n");
    printf("2) 댓글 작성\n");
    printf("3) 게시글 좋아요\n");
    printf("4) 동아리 가입 신청\n");
    printf("0) 뒤로가기\n");
    printf("입력: ");
    if (scanf("%d", &choice) != 1) {
      while (getchar() != '\n')
        ;
      continue;
    }

    if (choice == 0) {
      break;
    } else if (choice == 3) {
      if (like_post(conn, post_id, logged_id)) {
        printf("✅ 게시글에 좋아요를 눌렀습니다!\n");
      }
    } else if (choice == 4) {
      if (club_id > 0) {
        apply_for_club(conn, club_id, logged_id);
      } else {
        printf("❌ 동아리 정보를 찾을 수 없습니다.\n");
      }
    } else if (choice == 1) {
      int comment_ids[100];
      int comment_count = 0;
      get_post_comments_list(conn, post_id, comment_ids, &comment_count);

      if (comment_count > 0) {
        int comment_num;
        printf("\n상세보기 할 댓글 번호 입력 (0: 뒤로가기): ");
        if (scanf("%d", &comment_num) != 1) {
          while (getchar() != '\n')
            ;
          continue;
        }
        if (comment_num > 0 && comment_num <= comment_count) {
          view_comment_detail_menu(conn, comment_ids[comment_num - 1],
                                   logged_id);
        } else if (comment_num != 0) {
          printf("잘못된 번호입니다.\n");
        }
      }
    } else if (choice == 2) {
      char c_content[500];
      printf("댓글 내용 입력: ");
      while (getchar() != '\n')
        ;
      fgets(c_content, sizeof(c_content), stdin);
      c_content[strcspn(c_content, "\n")] = '\0';

      // 비속어 필터링 검사
      if (contains_slang(c_content)) {
        printf("❌ 부적절한 단어(욕설 등)가 포함되어 있어 댓글을 등록할 수 "
               "없습니다.\n");
        increment_bad_word_count(conn, logged_id);
        continue;
      }

      if (insert_comment(conn, post_id, logged_id, c_content, 0)) {
        printf("✅ 댓글이 등록되었습니다!\n");
      } else {
        printf("❌ 댓글 등록에 실패했습니다.\n");
      }
    } else {
      printf("잘못된 입력입니다.\n");
    }
  }
}

// ─────────────────────────────────────────────
// 최상위 댓글 목록 출력 및 일련번호 부여
// ─────────────────────────────────────────────
void get_post_comments_list(MYSQL *conn, int post_id, int *comment_ids_out,
                            int *comment_count_out) {
  char query[512];
  sprintf(query,
          "SELECT c.comment_id, u.nickname, c.content, c.created_at "
          "FROM comments c JOIN users u ON c.user_idx = u.user_idx "
          "WHERE c.post_id = %d AND c.parent_comment_id IS NULL "
          "ORDER BY c.created_at ASC",
          post_id);
  if (mysql_query(conn, query)) {
    printf("댓글 로딩 실패: %s\n", mysql_error(conn));
    return;
  }
  MYSQL_RES *res = mysql_store_result(conn);
  if (res == NULL)
    return;

  int count = mysql_num_rows(res);
  if (count == 0) {
    printf("\n(작성된 댓글이 없습니다)\n");
    *comment_count_out = 0;
    mysql_free_result(res);
    return;
  }

  printf("\n[댓글 목록]\n");
  printf("--------------------------------------------------\n");

  int idx = 0;
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res))) {
    int c_id = atoi(row[0]);
    comment_ids_out[idx] = c_id;
    int likes = get_comment_likes_count(conn, c_id);

    printf("[%d] %s (좋아요: %d) | %s\n", idx + 1, row[1], likes, row[3]);
    printf("    내용: %s\n", row[2]);

    // 대댓글 목록 들여쓰기로 출력
    char sub_query[512];
    sprintf(sub_query,
            "SELECT u.nickname, c.content, c.created_at "
            "FROM comments c JOIN users u ON c.user_idx = u.user_idx "
            "WHERE c.parent_comment_id = %d "
            "ORDER BY c.created_at ASC",
            c_id);
    MYSQL_RES *sub_res = NULL;
    if (mysql_query(conn, sub_query) == 0) {
      sub_res = mysql_store_result(conn);
    }
    if (sub_res) {
      MYSQL_ROW sub_row;
      while ((sub_row = mysql_fetch_row(sub_res))) {
        printf("    └─ [대댓글] %s | %s\n", sub_row[0], sub_row[2]);
        printf("         내용: %s\n", sub_row[1]);
      }
      mysql_free_result(sub_res);
    }
    printf("--------------------------------------------------\n");
    idx++;
  }

  *comment_count_out = idx;
  mysql_free_result(res);
}

// ─────────────────────────────────────────────
// 특정 댓글의 상세 메뉴 루프 (대댓글 작성, 좋아요)
// ─────────────────────────────────────────────
void view_comment_detail_menu(MYSQL *conn, int comment_id,
                              const char *logged_id) {
  int choice;
  while (1) {
    if (is_currently_suspended(conn, logged_id)) return;
    char query[512];
    sprintf(query,
            "SELECT u.nickname, c.content, c.created_at "
            "FROM comments c JOIN users u ON c.user_idx = u.user_idx "
            "WHERE c.comment_id = %d",
            comment_id);
    if (mysql_query(conn, query)) {
      printf("댓글을 로딩하지 못했습니다.\n");
      return;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL || mysql_num_rows(res) == 0) {
      if (res)
        mysql_free_result(res);
      printf("존재하지 않는 댓글입니다.\n");
      return;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    int likes = get_comment_likes_count(conn, comment_id);

    printf("\n=== 댓글 상세 정보 ===\n");
    printf("작성자: %s | 좋아요: %d | 작성일: %s\n", row[0], likes, row[2]);
    printf("내용: %s\n", row[1]);

    printf("----------------------------------\n");
    printf("[대댓글 목록]\n");
    char sub_query[512];
    sprintf(sub_query,
            "SELECT u.nickname, c.content, c.created_at "
            "FROM comments c JOIN users u ON c.user_idx = u.user_idx "
            "WHERE c.parent_comment_id = %d "
            "ORDER BY c.created_at ASC",
            comment_id);
    MYSQL_RES *sub_res = NULL;
    if (mysql_query(conn, sub_query) == 0) {
      sub_res = mysql_store_result(conn);
    }
    if (sub_res) {
      MYSQL_ROW sub_row;
      if (mysql_num_rows(sub_res) == 0) {
        printf("(대댓글이 없습니다)\n");
      } else {
        while ((sub_row = mysql_fetch_row(sub_res))) {
          printf("└─ %s | %s\n", sub_row[0], sub_row[2]);
          printf("   내용: %s\n", sub_row[1]);
        }
      }
      mysql_free_result(sub_res);
    }
    mysql_free_result(res);
    printf("----------------------------------\n");

    printf("1) 댓글에 댓글 달기\n");
    printf("2) 좋아요\n");
    printf("0) 뒤로가기\n");
    printf("입력: ");
    if (scanf("%d", &choice) != 1) {
      while (getchar() != '\n')
        ;
      continue;
    }

    if (choice == 0) {
      break;
    } else if (choice == 1) {
      char sub_content[500];
      printf("대댓글 입력: ");
      while (getchar() != '\n')
        ;
      fgets(sub_content, sizeof(sub_content), stdin);
      sub_content[strcspn(sub_content, "\n")] = '\0';

      // 비속어 필터링 검사
      if (contains_slang(sub_content)) {
        printf("❌ 부적절한 단어(욕설 등)가 포함되어 있어 대댓글을 등록할 수 "
               "없습니다.\n");
        increment_bad_word_count(conn, logged_id);
        continue;
      }

      // comments 테이블용 post_id 조회
      char post_id_query[256];
      sprintf(post_id_query,
              "SELECT post_id FROM comments WHERE comment_id = %d", comment_id);
      int post_id = 0;
      if (mysql_query(conn, post_id_query) == 0) {
        MYSQL_RES *p_res = mysql_store_result(conn);
        if (p_res) {
          MYSQL_ROW p_row = mysql_fetch_row(p_res);
          if (p_row)
            post_id = atoi(p_row[0]);
          mysql_free_result(p_res);
        }
      }

      if (post_id > 0 &&
          insert_comment(conn, post_id, logged_id, sub_content, comment_id)) {
        printf("✅ 대댓글이 성공적으로 등록되었습니다!\n");
      } else {
        printf("❌ 대댓글 등록에 실패했습니다.\n");
      }
    } else if (choice == 2) {
      if (has_user_liked_comment(conn, comment_id, logged_id)) {
        printf("⚠️ 이미 좋아요를 누른 댓글입니다.\n");
      } else {
        if (insert_comment_like(conn, comment_id, logged_id)) {
          printf("✅ 좋아요를 눌렀습니다!\n");
        } else {
          printf("❌ 좋아요 처리에 실패했습니다.\n");
        }
      }
    } else {
      printf("잘못된 입력입니다.\n");
    }
  }
}

void apply_for_club(MYSQL *conn, int club_id, const char *logged_id) {
    char query[1024];
    
    // 1. 가입자의 전공, 타겟 동아리의 카테고리(전공 여부) 및 동아리의 명시적 전공 조회
    sprintf(query, 
        "SELECT c.category_id, c.college_code, c.major_code, u_applicant.college_code, u_applicant.major_code "
        "FROM clubs c "
        "JOIN users u_applicant ON u_applicant.id = '%s' "
        "WHERE c.club_id = %d", logged_id, club_id);
        
    if (mysql_query(conn, query)) { 
        fprintf(stderr, "조회 실패: %s\n", mysql_error(conn));
        return; 
    }
    
    MYSQL_RES *res = mysql_store_result(conn);
    if (!res || mysql_num_rows(res) == 0) {
        if(res) mysql_free_result(res);
        printf("❌ 동아리 정보를 찾을 수 없습니다.\n");
        return;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    int category_id = atoi(row[0]);
    int c_college = row[1] ? atoi(row[1]) : 0;
    int c_major = row[2] ? atoi(row[2]) : 0;
    int u_college = row[3] ? atoi(row[3]) : 0;
    int u_major = row[4] ? atoi(row[4]) : 0;
    
    // 카테고리가 1(전공 동아리)일 경우 전공 일치 여부 확인
    if (category_id == 1) {
        if (c_college != u_college || c_major != u_major) {
            printf("❌ 가입 실패: 해당 동아리의 전공(%s)과 본인의 전공(%s)이 일치하지 않아 신청할 수 없습니다.\n", 
                   get_major_name(c_college, c_major), get_major_name(u_college, u_major));
            mysql_free_result(res);
            return;
        }
    }
    mysql_free_result(res);

    // 2. 기존 가입 신청 여부 확인
    sprintf(query, "SELECT 1 FROM joinrequests WHERE club_id = %d AND user_idx = (SELECT user_idx FROM users WHERE id='%s')", club_id, logged_id);
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *chk_res = mysql_store_result(conn);
        if (chk_res && mysql_num_rows(chk_res) > 0) {
            printf("⚠️ 이미 이 동아리에 가입 신청을 하셨습니다.\n");
            mysql_free_result(chk_res);
            return;
        }
        if (chk_res) mysql_free_result(chk_res);
    }
    
    // 3. 가입 동기 입력
    char intro[500];
    printf("가입 동기(자기소개)를 입력하세요: ");
    while(getchar() != '\n'); // 버퍼 비우기
    fgets(intro, sizeof(intro), stdin);
    intro[strcspn(intro, "\n")] = '\0';
    
    // 4. joinrequests 테이블에 INSERT 진행
    char esc_intro[1000];
    mysql_real_escape_string(conn, esc_intro, intro, strlen(intro));

    sprintf(query, "INSERT INTO joinrequests (club_id, user_idx, introduction) VALUES (%d, (SELECT user_idx FROM users WHERE id='%s'), '%s')", club_id, logged_id, esc_intro);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "❌ 신청 실패: %s\n", mysql_error(conn));
    } else {
        printf("✅ 성공적으로 가입 신청이 접수되었습니다!\n");
    }
}
