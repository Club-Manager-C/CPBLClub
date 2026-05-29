#include "board.h"
#include "filter.h"
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
  int choice;
  while (1) {
    if (is_currently_suspended(conn, logged_id)) return;
    printf("\n=========================================\n");
    printf("   %s\n",
           category_id == 1 ? "동아리 홍보 게시판" : "전공 동아리 게시판");
    printf("=========================================\n");
    get_posts_by_category(conn, category_id);

    printf("\n1. 글 상세 보기\n");
    printf("2. 글 작성\n");
    printf("0. 뒤로가기\n");
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
      if (category_id == 2 && !is_user_club_leader(conn, logged_id)) {
        printf("❌ 글 작성 권한이 없습니다. (전공 동아리장만 작성할 수 "
               "있습니다.)\n");
      } else {
        write_post_menu(conn, category_id, logged_id);
      }
    } else {
      printf("잘못된 입력입니다.\n");
    }
  }
}

// ─────────────────────────────────────────────
// 게시글 작성 화면
// ─────────────────────────────────────────────
void write_post_menu(MYSQL *conn, int category_id, const char *logged_id) {
  char title[200];
  char content[1000];

  printf("\n=== 새 글 작성 ===\n");
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

  if (insert_post(conn, logged_id, category_id, title, content)) {
    printf("✅ 글이 성공적으로 등록되었습니다!\n");
  } else {
    printf("❌ 글 등록에 실패했습니다.\n");
  }
}

// ─────────────────────────────────────────────
// 게시글 본문 상세 정보 출력
// ─────────────────────────────────────────────
int print_post_detail(MYSQL *conn, int post_id) {
  char query[512];
  sprintf(query,
          "SELECT p.title, p.content, u.nickname, p.created_at "
          "FROM posts p JOIN users u ON p.user_idx = u.user_idx "
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
  printf("\n==================================================\n");
  printf(" 제목: %s\n", row[0]);
  printf(" 작성자: %s | 작성일: %s\n", row[2], row[3]);
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
  while (1) {
    if (is_currently_suspended(conn, logged_id)) return;
    if (!print_post_detail(conn, post_id)) {
      printf("존재하지 않는 게시글입니다.\n");
      return;
    }

    printf("1) 댓글 확인\n");
    printf("2) 댓글 작성\n");
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
