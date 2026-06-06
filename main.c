#include "auth.h"
#include "board.h"
#include "db.h"
#include "mypage.h"
#include "apply_period.h"
#include "filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "major_info.h"

void run_admin_interface(MYSQL *conn);
void view_clubs_by_category(MYSQL *conn, const char *logged_id);

// ─────────────────────────────────────────────
// 동아리장 신청 (스텁)
// ─────────────────────────────────────────────
void apply_club_leader(MYSQL *conn, const char *logged_id) {
  printf("\n=== 동아리장 및 동아리 신청 ===\n");

  char club_name[100];
  int category_id;
  char description[300];
  char professor_name[50];
  char operating_hours[100];

  printf("동아리 이름: ");
  scanf("%99s", club_name);

  // 카테고리 로딩
  printf("\n[카테고리 목록]\n");
  if (mysql_query(conn,
                  "SELECT category_id, category_name FROM club_categories ORDER BY category_id ASC")) {
    printf("카테고리 로딩 실패: %s\n", mysql_error(conn));
    return;
  }
  MYSQL_RES *res = mysql_store_result(conn);
  MYSQL_ROW row;
  int cat_ids[100];
  int cat_count = 0;
  while ((row = mysql_fetch_row(res))) {
    cat_ids[cat_count] = atoi(row[0]);
    printf("%d. %s\n", cat_count + 1, row[1]);
    cat_count++;
  }
  mysql_free_result(res);

  int choice;
  printf("카테고리 번호 선택: ");
  scanf("%d", &choice);
  if (choice < 1 || choice > cat_count) {
    printf("잘못된 선택입니다.\n");
    return;
  }
  category_id = cat_ids[choice - 1];

  int college_code = 0, major_code = 0;
  if (category_id == 1) { // 1 is '전공' 카테고리
      select_college_and_major(&college_code, &major_code);
  }

  printf("지도 교수님 이름: ");
  scanf("%49s", professor_name);
  while (getchar() != '\n'); // scanf 직후 입력 버퍼 즉시 비우기 (중요!)

  printf("활동 시간대 (예: 매주 목요일 18:00): ");
  fgets(operating_hours, sizeof(operating_hours), stdin);
  operating_hours[strcspn(operating_hours, "\n")] = 0; // 개행 제거

  printf("동아리의 목적 및 설명 (300자 이내): ");
  fgets(description, sizeof(description), stdin);
  description[strcspn(description, "\n")] = 0; // 개행 제거

  // 비속어 필터링 검사
  if (contains_slang(club_name) || contains_slang(description)) {
    printf("\n❌ 부적절한 단어(욕설 등)가 포함되어 있어 동아리 신청을 할 수 없습니다.\n");
    increment_bad_word_count(conn, logged_id);
    return;
  }

  // 문자열 SQL 인젝션 방지를 위한 안전한 이스케이프
  char esc_club_name[200], esc_prof[100], esc_hours[200], esc_desc[600], esc_logged_id[100];
  mysql_real_escape_string(conn, esc_club_name, club_name, strlen(club_name));
  mysql_real_escape_string(conn, esc_prof, professor_name, strlen(professor_name));
  mysql_real_escape_string(conn, esc_hours, operating_hours, strlen(operating_hours));
  mysql_real_escape_string(conn, esc_desc, description, strlen(description));
  mysql_real_escape_string(conn, esc_logged_id, logged_id, strlen(logged_id));

  // DB에 삽입 (status는 기본값 '대기'로 저장)
  char query[2048];
  if (category_id == 1) {
      sprintf(query,
              "INSERT INTO clubs (club_name, category_id, college_code, major_code, description, professor_name, "
              "operating_hours, leader_idx, status) "
              "VALUES ('%s', %d, %d, %d, '%s', '%s', '%s', (SELECT user_idx FROM users WHERE id = '%s'), '대기')",
              esc_club_name, category_id, college_code, major_code, esc_desc, esc_prof, esc_hours, esc_logged_id);
  } else {
      sprintf(query,
              "INSERT INTO clubs (club_name, category_id, description, professor_name, "
              "operating_hours, leader_idx, status) "
              "VALUES ('%s', %d, '%s', '%s', '%s', (SELECT user_idx FROM users WHERE id = '%s'), '대기')",
              esc_club_name, category_id, esc_desc, esc_prof, esc_hours, esc_logged_id);
  }

  if (mysql_query(conn, query)) {
    printf("신청에 실패했습니다: %s\n", mysql_error(conn));
  } else {
    printf("\n동아리장 및 동아리 신청이 성공적으로 접수되었습니다!\n");
    printf("- 신청 동아리명: %s\n", club_name);
    printf("- 지도 교수님: %s\n", professor_name);
    printf("- 승인 상태: 대기중 (관리자 승인 대기)\n");
  }
}

// ─────────────────────────────────────────────
// 카테고리별 동아리 목록 보기
// ─────────────────────────────────────────────
void view_clubs_by_category(MYSQL *conn, const char *logged_id) {
  printf("\n=== 동아리 카테고리 목록 ===\n");
  if (mysql_query(conn,
                  "SELECT category_id, category_name FROM club_categories ORDER BY category_id ASC")) {
    printf("카테고리 로딩 실패: %s\n", mysql_error(conn));
    return;
  }
  MYSQL_RES *res = mysql_store_result(conn);
  MYSQL_ROW row;
  int cat_ids[100];
  int cat_count = 0;
  while ((row = mysql_fetch_row(res))) {
    cat_ids[cat_count] = atoi(row[0]);
    printf("%d. %s\n", cat_count + 1, row[1]);
    cat_count++;
  }
  mysql_free_result(res);

  int choice;
  printf("\n조회할 카테고리 번호 선택 (0. 뒤로가기): ");
  scanf("%d", &choice);

  if (choice == 0)
    return;

  if (choice < 1 || choice > cat_count) {
    printf("잘못된 선택입니다.\n");
    return;
  }
  int category_id = cat_ids[choice - 1];

  // 기존에는 동아리 목록을 보여주었으나, 이제는 해당 카테고리의 게시판으로 직접 이동
  show_board_menu(conn, category_id, logged_id);
}

// ─────────────────────────────────────────────
// 메인 화면 (로그인 후 진입)
// ─────────────────────────────────────────────
void home_screen(MYSQL *conn, const char *logged_id) {
  int choice;

  while (1) {
    if (is_currently_suspended(conn, logged_id)) return;
    printf("\n============================\n");
    printf("  메인 메뉴\n");
    printf("============================\n");
    printf("1. 동아리 목록 (카테고리별)\n");
    printf("2. 마이페이지\n");
    printf("3. 동아리 개설 신청\n");
    printf("0. 로그아웃\n");
    printf("============================\n");
    printf("입력: ");
    scanf("%d", &choice);

    switch (choice) {
    case 1:
      view_clubs_by_category(conn, logged_id);
      break;
    case 2:
      my_page(conn, logged_id);
      break;
    case 3:
      // 신청 기간 확인 후 동아리장 신청 실행
      if (is_apply_period_open(conn)) 
      {   // 신청 기간 열려있음
          apply_club_leader(conn, logged_id);

      } else {
          // 신청 기간 아님
          printf("\n현재는 동아리 등록 신청 기간이 아닙니다.\n");
      }
      break;
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

  // 비속어 필터 로드
  load_slang_list("slang_list.txt");

  int choice;
  char logged_id[50];

  while (1) {
    printf("\n=== 시작 화면 ===\n");
    printf("1. 로그인\n");
    printf("2. 회원가입\n");
    printf("0. 종료\n");
    printf("=====================\n");
    printf("입력: ");
    scanf("%d", &choice);

    if (choice == 1) {
      int login_status = login_screen(conn, logged_id);
      if (login_status == 1) {
        home_screen(conn, logged_id);
      } else if (login_status == 2) {
        run_admin_interface(conn);
      }
    } else if (choice == 2) {
      register_screen(conn);
    } else if (choice == 0) {
      printf("프로그램을 종료합니다.\n");
      break;
    } else {
      printf("잘못된 입력입니다.\n");
    }
  }

  close_db(conn);
  free_slang_list();
  return 0;
}
