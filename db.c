#include "db.h"
#include "major_info.h"
#include "decorate.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


const char *DB_HOST = "localhost";
const char *DB_USER = "root";
const char *DB_PW = "kim7479050810#";
const char *DB_NAME = "cpbl_db";

void init_db(MYSQL **conn) {
  *conn = mysql_init(NULL);
  if (*conn == NULL) {
    fprintf(stderr, "mysql_init() 실패\n");
    exit(1);
  }

  // DB 서버 접속 전 SSL 옵션 끄기 (수정된 코드)
  enum mysql_ssl_mode ssl_mode = SSL_MODE_DISABLED;
  mysql_options(*conn, MYSQL_OPT_SSL_MODE, &ssl_mode);

  if (mysql_real_connect(*conn, DB_HOST, DB_USER, DB_PW, NULL, 3306, NULL, 0) ==
      NULL) {
    fprintf(stderr, "MySQL 연결 실패: %s\n", mysql_error(*conn));
    exit(1);
  }

  // 한글 깨짐 방지: 클라이언트 연결 인코딩을 utf8로 설정
  mysql_set_character_set(*conn, "utf8");

  mysql_query(*conn, "CREATE DATABASE IF NOT EXISTS cpbl_db");
  mysql_select_db(*conn, DB_NAME);

  // ① 동아리 카테고리 테이블
  mysql_query(*conn, "CREATE TABLE IF NOT EXISTS club_categories ("
                     "category_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, "
                     "category_name VARCHAR(50) NOT NULL UNIQUE, "
                     "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");

  // ② 동아리 등록 기간 관리 테이블
  mysql_query(*conn, "CREATE TABLE IF NOT EXISTS recruitment_periods ("
                     "period_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, "
                     "label VARCHAR(50) NOT NULL, "
                     "start_date DATE NOT NULL, "
                     "end_date DATE NOT NULL, "
                     "is_active TINYINT(1) DEFAULT 1)");

  // ③ 사용자 테이블 (총관리자 권한 및 동아리장 플래그 포함)
  mysql_query(*conn, "CREATE TABLE IF NOT EXISTS users ("
                     "user_idx INT NOT NULL AUTO_INCREMENT PRIMARY KEY, "
                     "id VARCHAR(20) NOT NULL UNIQUE, "
                     "pw VARCHAR(255) NOT NULL, "
                     "nickname VARCHAR(30) NOT NULL UNIQUE, "
                     "student_id VARCHAR(10) NOT NULL UNIQUE, "
                     "college_code INT NOT NULL, "
                     "major_code INT NOT NULL, "
                     "name VARCHAR(20) NOT NULL, "
                     "phone VARCHAR(15) NOT NULL UNIQUE, "
                     "role ENUM('User', 'Admin') NOT NULL DEFAULT 'User', "
                     "is_approved TINYINT(1) DEFAULT 0, "
                     "bad_word_count INT DEFAULT 0, "
                     "is_suspended TINYINT DEFAULT 0, "
                     "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");

  // 마이그레이션: 기존 사용자 테이블에 새 필드 추가 (이미 존재하면 무시됨)
  mysql_query(*conn,
              "ALTER TABLE users ADD COLUMN bad_word_count INT DEFAULT 0");
  mysql_query(*conn,
              "ALTER TABLE users ADD COLUMN is_suspended TINYINT DEFAULT 0");

  // ④ 동아리 테이블 (leader_idx를 통해 현재 주인 식별)
  mysql_query(
      *conn,
      "CREATE TABLE IF NOT EXISTS clubs ("
      "club_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, "
      "club_name VARCHAR(50) NOT NULL UNIQUE, "
      "category_id INT NOT NULL, "
      "description TEXT NULL, "
      "professor_name VARCHAR(20) NULL, "
      "operating_hours VARCHAR(100) NULL, "
      "status ENUM('대기', '승인', '거절') DEFAULT '대기', "
      "reject_reason VARCHAR(255) NULL, "
      "college_code INT NULL, "
      "major_code INT NULL, "
      "leader_idx INT NULL, "
      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
      "FOREIGN KEY (category_id) REFERENCES club_categories (category_id), "
      "FOREIGN KEY (leader_idx) REFERENCES users (user_idx) ON DELETE SET "
      "NULL)");

  // (마이그레이션 제거됨)

  // ⑤ 동아리 멤버 테이블 (N:M 관계 해소 및 마이페이지 직책 표시용)
  mysql_query(
      *conn,
      "CREATE TABLE IF NOT EXISTS clubmembers ("
      "membership_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, "
      "club_id INT NOT NULL, "
      "user_idx INT NOT NULL, "
      "role ENUM('Member', 'Leader') DEFAULT 'Member', "
      "joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
      "UNIQUE KEY unique_membership (club_id, user_idx), "
      "FOREIGN KEY (club_id) REFERENCES clubs (club_id) ON DELETE CASCADE, "
      "FOREIGN KEY (user_idx) REFERENCES users (user_idx) ON DELETE CASCADE)");

  // ⑥ 게시판 타입 테이블 (운영 정책 반영)
  mysql_query(*conn, "CREATE TABLE IF NOT EXISTS boardtype ("
                     "type_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, "
                     "board_name VARCHAR(20) NOT NULL, "
                     "min_role ENUM('Member', 'Leader') DEFAULT 'Member')");

  // ⑦ 게시글 테이블 (동아리나 사용자 삭제 시 관련 글 자동 삭제)
  mysql_query(
      *conn,
      "CREATE TABLE IF NOT EXISTS posts ("
      "post_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, "
      "club_id INT NOT NULL, "
      "user_idx INT NOT NULL, "
      "type_id INT NOT NULL, "
      "title VARCHAR(255) NOT NULL, "
      "content TEXT NOT NULL, "
      "view_count INT DEFAULT 0, "
      "like_count INT DEFAULT 0, "
      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
      "FOREIGN KEY (club_id) REFERENCES clubs (club_id) ON DELETE CASCADE, "
      "FOREIGN KEY (user_idx) REFERENCES users (user_idx) ON DELETE CASCADE, "
      "FOREIGN KEY (type_id) REFERENCES boardtype (type_id))");

  // ⑧ 가입 신청 테이블 (등록 기간 연동 및 승인 로직용)
  mysql_query(
      *conn,
      "CREATE TABLE IF NOT EXISTS joinrequests ("
      "request_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, "
      "club_id INT NOT NULL, "
      "user_idx INT NOT NULL, "
      "introduction TEXT NOT NULL, "
      "status ENUM('대기', '승인', '거절') DEFAULT '대기', "
      "period_id INT NULL, "
      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
      "FOREIGN KEY (club_id) REFERENCES clubs (club_id) ON DELETE CASCADE, "
      "FOREIGN KEY (user_idx) REFERENCES users (user_idx) ON DELETE CASCADE, "
      "FOREIGN KEY (period_id) REFERENCES recruitment_periods (period_id))");

  // ⑨ 메시지/알림함 테이블 (개설 및 가입 승인 알림용)
  mysql_query(*conn, "CREATE TABLE IF NOT EXISTS messages ("
                     "message_id INT AUTO_INCREMENT PRIMARY KEY, "
                     "receiver_idx INT NOT NULL, "
                     "contented_at TEXT NOT NULL, "
                     "is_read TINYINT(1) DEFAULT 0, "
                     "sent_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                     "FOREIGN KEY (receiver_idx) REFERENCES users (user_idx) "
                     "ON DELETE CASCADE)");

  // 마이그레이션: 메시지 테이블에 쪽지/공지 구분을 위한 필드 추가
  mysql_query(*conn,
              "ALTER TABLE messages ADD COLUMN msg_type TINYINT DEFAULT 1");
  mysql_query(*conn, "ALTER TABLE messages ADD COLUMN sender_info VARCHAR(50) "
                     "DEFAULT 'System'");

  // ⑩ 댓글 테이블 (대댓글 구조 포함)
  mysql_query(
      *conn,
      "CREATE TABLE IF NOT EXISTS comments ("
      "comment_id INT AUTO_INCREMENT PRIMARY KEY, "
      "post_id INT NOT NULL, "
      "user_idx INT NOT NULL, "
      "content TEXT NOT NULL, "
      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
      "parent_comment_id INT DEFAULT NULL, "
      "FOREIGN KEY (post_id) REFERENCES posts (post_id) ON DELETE CASCADE, "
      "FOREIGN KEY (user_idx) REFERENCES users (user_idx) ON DELETE CASCADE, "
      "FOREIGN KEY (parent_comment_id) REFERENCES comments (comment_id) ON "
      "DELETE CASCADE)");

  // 기존 버전에 parent_comment_id가 없을 경우를 대비한 마이그레이션 코드 (이미
  // 존재하면 쿼리 실패하며 무시됨)
  mysql_query(
      *conn,
      "ALTER TABLE comments ADD COLUMN parent_comment_id INT DEFAULT NULL");
  mysql_query(
      *conn,
      "ALTER TABLE comments ADD CONSTRAINT fk_parent_comment FOREIGN KEY "
      "(parent_comment_id) REFERENCES comments(comment_id) ON DELETE CASCADE");

  // ⑩-1. 댓글 좋아요 테이블 생성
  mysql_query(
      *conn,
      "CREATE TABLE IF NOT EXISTS comment_likes ("
      "comment_id INT NOT NULL, "
      "user_idx INT NOT NULL, "
      "PRIMARY KEY (comment_id, user_idx), "
      "FOREIGN KEY (comment_id) REFERENCES comments (comment_id) ON DELETE "
      "CASCADE, "
      "FOREIGN KEY (user_idx) REFERENCES users (user_idx) ON DELETE CASCADE)");

  // ⑪ 비속어 필터링 테이블
  mysql_query(*conn, "CREATE TABLE IF NOT EXISTS filterwords ("
                     "word_id INT AUTO_INCREMENT PRIMARY KEY, "
                     "bad_word VARCHAR(50) NOT NULL UNIQUE)");

  // ⑫ 시간표 테이블 (ENUM 활용)
  mysql_query(
      *conn,
      "CREATE TABLE IF NOT EXISTS timetables ("
      "table_id INT AUTO_INCREMENT PRIMARY KEY, "
      "user_idx INT NOT NULL, "
      "subject_name VARCHAR(50), "
      "day_of_week ENUM('월', '화', '수', '목', '금', '토', '일'), "
      "start_time TIME, "
      "end_time TIME, "
      "FOREIGN KEY (user_idx) REFERENCES users (user_idx) ON DELETE CASCADE)");

  // ⑬ 검색 로그 테이블 (사용자 본인의 기록 조회용)
  mysql_query(
      *conn,
      "CREATE TABLE IF NOT EXISTS searchlogs ("
      "log_id INT AUTO_INCREMENT PRIMARY KEY, "
      "user_idx INT NOT NULL, "
      "keyword VARCHAR(100) NOT NULL, "
      "searched_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
      "FOREIGN KEY (user_idx) REFERENCES users (user_idx) ON DELETE CASCADE)");

  // ──────── 시드 데이터 삽입 ─────────────────────────────────
  mysql_query(
      *conn,
      "INSERT IGNORE INTO club_categories (category_id, category_name) VALUES "
      "(1, '전공'), (2, '밴드'), (3, '댄스'), (4, '봉사'), (5, '취미'), (6, "
      "'운동')");

  mysql_query(
      *conn,
      "INSERT IGNORE INTO users (user_idx, id, pw, nickname, "
      "student_id, college_code, major_code, name, phone, role, is_approved) "
      "VALUES (1, 'admin', 'admin1234', '관리자', '00000000', "
      "1, 1, '관리자', '010-0000-0000', 'Admin', 1)");

  mysql_query(*conn, "INSERT IGNORE INTO clubs (club_id, club_name, "
                     "category_id, leader_idx, status, description) "
                     "VALUES (1, '기본 동아리', 1, 1, '승인', '시스템 기본 "
                     "설정 동아리입니다.')");

  mysql_query(*conn,
              "INSERT IGNORE INTO boardtype (type_id, board_name) VALUES (1, "
              "'동아리 홍보'), (2, '전공 동아리'), (3, '동아리 공지사항')");
}

int check_login(MYSQL *conn, const char *id, const char *pw) {
  char query[256];
  sprintf(query, "SELECT pw, is_suspended FROM users WHERE id = '%s'", id);

  if (mysql_query(conn, query)) {
    fprintf(stderr, "쿼리 실패: %s\n", mysql_error(conn));
    return 0; // 실패
  }

  MYSQL_RES *res = mysql_store_result(conn);
  if (res != NULL && mysql_num_rows(res) > 0) {
    MYSQL_ROW row = mysql_fetch_row(res);
    if (strcmp(row[0], pw) == 0) {
      int is_suspended = atoi(row[1]);
      mysql_free_result(res);
      if (is_suspended == 1) {
        printf("정지된 계정입니다.\n");
        return -1; // 정지된 계정 코드
      }
      return 1; // 성공
    }
  }
  if (res != NULL)
    mysql_free_result(res);
  return 0; // 실패
}

int check_id_duplicate(MYSQL *conn, const char *id) {
  char query[256];
  sprintf(query, "SELECT id FROM users WHERE id = '%s'", id);
  if (mysql_query(conn, query) == 0) {
    MYSQL_RES *res = mysql_store_result(conn);
    if (res != NULL && mysql_num_rows(res) > 0) {
      mysql_free_result(res);
      return 1; // 중복됨
    }
    if (res != NULL)
      mysql_free_result(res);
  }
  return 0; // 중복 안됨 (사용 가능)
}

int check_nickname_duplicate(MYSQL *conn, const char *nickname) {
  char query[256];
  sprintf(query, "SELECT nickname FROM users WHERE nickname = '%s'", nickname);
  if (mysql_query(conn, query) == 0) {
    MYSQL_RES *res = mysql_store_result(conn);
    if (res != NULL && mysql_num_rows(res) > 0) {
      mysql_free_result(res);
      return 1; // 중복됨
    }
    if (res != NULL)
      mysql_free_result(res);
  }
  return 0; // 중복 안됨 (사용 가능)
}

int check_student_id_duplicate(MYSQL *conn, long long student_id) {
  char query[256];
  sprintf(query, "SELECT student_id FROM users WHERE student_id = '%lld'",
          student_id);
  if (mysql_query(conn, query) == 0) {
    MYSQL_RES *res = mysql_store_result(conn);
    if (res != NULL && mysql_num_rows(res) > 0) {
      mysql_free_result(res);
      return 1; // 중복됨
    }
    if (res != NULL)
      mysql_free_result(res);
  }
  return 0; // 중복 안됨
}

int register_user(MYSQL *conn, const char *id, const char *pw,
                  const char *nickname, long long student_id, const char *name,
                  int college_code, int major_code, const char *phone) {
  char query[768];
  sprintf(query,
          "INSERT INTO users (id, pw, nickname, student_id, name, "
          "college_code, major_code, phone) "
          "VALUES ('%s', '%s', '%s', '%lld', '%s', %d, %d, '%s')",
          id, pw, nickname, student_id, name, college_code, major_code, phone);

  if (mysql_query(conn, query)) {
    fprintf(stderr, "회원가입 실패: %s\n", mysql_error(conn));
    return 0; // 실패
  }
  return 1; // 성공
}

// ─────────────────────────────────────────────────
// 내 게시글 목록 출력
// ─────────────────────────────────────────────────
void get_my_posts(MYSQL *conn, const char *user_id) {
  char query[512];
  sprintf(query,
          "SELECT p.post_id, p.title, p.created_at FROM posts p "
          "JOIN users u ON p.user_idx = u.user_idx "
          "WHERE u.id = '%s' ORDER BY p.created_at DESC",
          user_id);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "쿼리 실패: %s\n", mysql_error(conn));
    return;
  }
  MYSQL_RES *res = mysql_store_result(conn);
  if (res == NULL)
    return;
  if (mysql_num_rows(res) == 0) {
    printf("작성한 게시글이 없습니다.\n");
    mysql_free_result(res);
    return;
  }
  printf("%-6s %-30s %-20s\n", "ID", "제목", "작성일");
  printf("------------------------------------------------------------\n");
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res))) {
    printf("[%-4s] %-30s %s\n", row[0], row[1], row[2]);
  }
  mysql_free_result(res);
}

// 게시글 내용 수정
int update_post(MYSQL *conn, int post_id, const char *user_id,
                const char *new_content) {
  char query[768];
  sprintf(query,
          "UPDATE posts SET content = '%s' "
          "WHERE post_id = %d AND user_idx = (SELECT user_idx FROM users WHERE "
          "id = '%s')",
          new_content, post_id, user_id);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "수정 실패: %s\n", mysql_error(conn));
    return 0;
  }
  return (int)mysql_affected_rows(conn) > 0 ? 1 : 0;
}

// 카테고리별 게시글 조회
void get_posts_by_category(MYSQL *conn, int category_id) {
  char query[512];
  sprintf(query,
          "SELECT p.post_id, p.title, u.name, u.college_code, u.major_code, "
          "c.club_name, c.college_code, c.major_code, p.created_at "
          "FROM posts p "
          "JOIN users u ON p.user_idx = u.user_idx "
          "JOIN clubs c ON p.club_id = c.club_id "
          "WHERE c.category_id = %d ORDER BY p.created_at DESC",
          category_id);

  if (mysql_query(conn, query)) {
    fprintf(stderr, "게시글 조회 실패: %s\n", mysql_error(conn));
    return;
  }

  MYSQL_RES *res = mysql_store_result(conn);
  if (res == NULL)
    return;

  if (mysql_num_rows(res) == 0) {
    printf("게시글이 없습니다.\n");
    mysql_free_result(res);
    return;
  }

  printf("\n");
  printf("%-6s ", "ID");
  print_fixed_utf8("제목", 33);
  printf(" %-15s %-12s\n", "작성자", "작성일");
  printf("----------------------------------------------------------------\n");

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res))) {
      int u_col = atoi(row[3]);
      int u_maj = atoi(row[4]);

      char date_only[11];
      strncpy(date_only, row[8], 10);
      date_only[10] = '\0';

      printf("[%-4s] ", row[0]);
      print_fixed_utf8(row[1], 34);
      printf(" %-10s %-12s\n", row[2], date_only);

      printf("       전공: %s | 동아리: %s\n\n",
            get_major_name(u_col, u_maj),
            row[5]);
  } 

printf("----------------------------------------------------------------\n");

  mysql_free_result(res);
}

int search_posts_by_keyword(MYSQL *conn, int category_id, const char *keyword) {
  if (!keyword)
    return 0;

  // SQL 인젝션 방지를 위한 안전한 이스케이프
  char esc_keyword[100];
  mysql_real_escape_string(conn, esc_keyword, keyword, strlen(keyword));

  char query[1024];
  sprintf(query,
          "SELECT p.post_id, p.title, u.name, u.college_code, u.major_code, "
          "c.club_name, c.college_code, c.major_code, p.created_at "
          "FROM posts p "
          "JOIN users u ON p.user_idx = u.user_idx "
          "JOIN clubs c ON p.club_id = c.club_id "
          "WHERE c.category_id = %d AND (p.title LIKE '%%%s%%' OR p.content "
          "LIKE '%%%s%%') "
          "ORDER BY p.created_at DESC",
          category_id, esc_keyword, esc_keyword);

  if (mysql_query(conn, query)) {
    fprintf(stderr, "게시글 검색 실패: %s\n", mysql_error(conn));
    return 0;
  }

  MYSQL_RES *res = mysql_store_result(conn);
  if (res == NULL)
    return 0;

  int row_count = mysql_num_rows(res);
  if (row_count == 0) {
    mysql_free_result(res);
    return 0;
  }

  printf("\n=== 검색 결과 목록 (최신순) ===\n");
  printf("%-6s %-40s %-45s %-20s\n", "ID", "제목", "작성자 정보", "작성일");
  printf("---------------------------------------------------------------------"
         "---------------------------------------------\n");
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res))) {
    char author_info[100];
    int u_col = atoi(row[3]);
    int u_maj = atoi(row[4]);
    sprintf(author_info, "[작성자: %s (%s) | 등록 동아리: %s]", row[2],
            get_major_name(u_col, u_maj), row[5]);

    char title_str[150];
    if (row[6] && row[7]) {
      int c_col = atoi(row[6]);
      int c_maj = atoi(row[7]);
      if (c_col > 0) {
        sprintf(title_str, "[%s-%s] %s", get_college_name(c_col),
                get_major_name(c_col, c_maj), row[1]);
      } else {
        strcpy(title_str, row[1]);
      }
    } else {
      strcpy(title_str, row[1]);
    }
    printf("[%-4s] %-40s %-45s %s\n", row[0], title_str, author_info, row[8]);
  }
  mysql_free_result(res);
  return row_count;
}

// ─────────────────────────────────────────────────
// 내 댓글 목록 출력
// ─────────────────────────────────────────────────
void get_my_comments(MYSQL *conn, const char *user_id) {
  char query[512];
  sprintf(query,
          "SELECT c.comment_id, p.title, c.content, c.created_at "
          "FROM comments c JOIN posts p ON c.post_id = p.post_id "
          "JOIN users u ON c.user_idx = u.user_idx "
          "WHERE u.id = '%s' ORDER BY c.created_at DESC",
          user_id);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "쿼리 실패: %s\n", mysql_error(conn));
    return;
  }
  MYSQL_RES *res = mysql_store_result(conn);
  if (res == NULL)
    return;
  if (mysql_num_rows(res) == 0) {
    printf("작성한 댓글이 없습니다.\n");
    mysql_free_result(res);
    return;
  }
  printf("%-6s %-20s %-30s %-20s\n", "ID", "게시글 제목", "댓글 내용",
         "작성일");
  printf("------------------------------------------------------------\n");
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res))) {
    printf("[%-4s] %-20s %-30s %s\n", row[0], row[1], row[2], row[3]);
  }
  mysql_free_result(res);
}

// 댓글 내용 수정
int update_comment(MYSQL *conn, int comment_id, const char *user_id,
                   const char *new_content) {
  char query[768];
  sprintf(query,
          "UPDATE comments SET content = '%s' "
          "WHERE comment_id = %d AND user_idx = (SELECT user_idx FROM users "
          "WHERE id = '%s')",
          new_content, comment_id, user_id);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "수정 실패: %s\n", mysql_error(conn));
    return 0;
  }
  return (int)mysql_affected_rows(conn) > 0 ? 1 : 0;
}

// ─────────────────────────────────────────────────
// 시간표 목록 출력
// ─────────────────────────────────────────────────
void get_my_schedule(MYSQL *conn, const char *user_id) {
  char query[512];
  sprintf(query,
          "SELECT t.table_id, t.day_of_week, t.start_time, t.end_time, "
          "t.subject_name "
          "FROM timetables t JOIN users u ON t.user_idx = u.user_idx "
          "WHERE u.id = '%s' "
          "ORDER BY FIELD(t.day_of_week,'월','화','수','목','금','토','일'), "
          "t.start_time",
          user_id);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "쿼리 실패: %s\n", mysql_error(conn));
    return;
  }
  MYSQL_RES *res = mysql_store_result(conn);
  if (res == NULL)
    return;
  if (mysql_num_rows(res) == 0) {
    printf("등록된 시간표가 없습니다.\n");
    mysql_free_result(res);
    return;
  }
  printf("%-6s %-5s %-7s %-7s %-20s\n", "ID", "요일", "시작", "종료", "과목명");
  printf("------------------------------------------------------------\n");
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res))) {
    printf("[%-4s] %-5s %-7s %-7s %-20s\n", row[0], row[1], row[2], row[3],
           row[4]);
  }
  mysql_free_result(res);
}

// 시간표 항목 추가
int add_schedule(MYSQL *conn, const char *user_id, const char *day,
                 const char *start, const char *end, const char *subject,
                 const char *location) {
  char query[512];
  sprintf(query,
          "INSERT INTO timetables "
          "(user_idx, day_of_week, start_time, end_time, subject_name) "
          "VALUES ((SELECT user_idx FROM users WHERE id = "
          "'%s'),'%s','%s','%s','%s')",
          user_id, day, start, end, subject);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "추가 실패: %s\n", mysql_error(conn));
    return 0;
  }
  return 1;
}

// 시간표 항목 삭제
int delete_schedule(MYSQL *conn, int schedule_id, const char *user_id) {
  char query[256];
  sprintf(query,
          "DELETE FROM timetables "
          "WHERE table_id = %d AND user_idx = (SELECT user_idx FROM users "
          "WHERE id = '%s')",
          schedule_id, user_id);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "삭제 실패: %s\n", mysql_error(conn));
    return 0;
  }
  return (int)mysql_affected_rows(conn) > 0 ? 1 : 0;
}

void close_db(MYSQL *conn) {
  if (conn) {
    mysql_close(conn);
  }
}

int insert_message(MYSQL *conn, const char *user_id, const char *content) {
  char esc_content[1024], esc_user_id[100];
  mysql_real_escape_string(conn, esc_content, content, strlen(content));
  mysql_real_escape_string(conn, esc_user_id, user_id, strlen(user_id));

  char query[2048];
  sprintf(query,
          "INSERT INTO messages (receiver_idx, contented_at) VALUES "
          "((SELECT user_idx FROM users WHERE id = '%s'), '%s')",
          esc_user_id, esc_content);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "알림 등록 실패: %s\n", mysql_error(conn));
    return 0;
  }
  return 1;
}

int get_user_idx_by_id(MYSQL *conn, const char *user_id) {
  char query[256];
  sprintf(query, "SELECT user_idx FROM users WHERE id = '%s'", user_id);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "user_idx 조회 실패: %s\n", mysql_error(conn));
    return -1;
  }
  MYSQL_RES *res = mysql_store_result(conn);
  if (res == NULL || mysql_num_rows(res) == 0) {
    if (res)
      mysql_free_result(res);
    return -1;
  }
  MYSQL_ROW row = mysql_fetch_row(res);
  int idx = atoi(row[0]);
  mysql_free_result(res);
  return idx;
}

// ─────────────────────────────────────────────────
// 신규 게시판 및 댓글 제어 함수군
// ─────────────────────────────────────────────────

int insert_post(MYSQL *conn, int club_id, const char *user_id, int category_id,
                const char *title, const char *content) {
  char esc_title[500], esc_content[2500], esc_user_id[100];
  mysql_real_escape_string(conn, esc_title, title, strlen(title));
  mysql_real_escape_string(conn, esc_content, content, strlen(content));
  mysql_real_escape_string(conn, esc_user_id, user_id, strlen(user_id));

  char query[4000];
  // type_id는 1(기본 게시판)로 고정하고, 카테고리 분류는 동아리의 category_id를
  // 통해 필터링합니다.
  sprintf(
      query,
      "INSERT INTO posts (club_id, user_idx, type_id, title, content) VALUES "
      "(%d, (SELECT user_idx FROM users WHERE id = '%s'), 1, '%s', '%s')",
      club_id, esc_user_id, esc_title, esc_content);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "게시글 작성 실패: %s\n", mysql_error(conn));
    return 0;
  }
  return 1;
}

int insert_comment(MYSQL *conn, int post_id, const char *user_id,
                   const char *content, int parent_comment_id) {
  char esc_content[2500], esc_user_id[100];
  mysql_real_escape_string(conn, esc_content, content, strlen(content));
  mysql_real_escape_string(conn, esc_user_id, user_id, strlen(user_id));

  char query[4000];
  if (parent_comment_id > 0) {
    sprintf(query,
            "INSERT INTO comments (post_id, user_idx, content, "
            "parent_comment_id) VALUES "
            "(%d, (SELECT user_idx FROM users WHERE id = '%s'), '%s', %d)",
            post_id, esc_user_id, esc_content, parent_comment_id);
  } else {
    sprintf(query,
            "INSERT INTO comments (post_id, user_idx, content, "
            "parent_comment_id) VALUES "
            "(%d, (SELECT user_idx FROM users WHERE id = '%s'), '%s', NULL)",
            post_id, esc_user_id, esc_content);
  }
  if (mysql_query(conn, query)) {
    fprintf(stderr, "댓글 작성 실패: %s\n", mysql_error(conn));
    return 0;
  }
  return 1;
}

int insert_comment_like(MYSQL *conn, int comment_id, const char *user_id) {
  char esc_user_id[100];
  mysql_real_escape_string(conn, esc_user_id, user_id, strlen(user_id));

  char query[512];
  sprintf(query,
          "INSERT INTO comment_likes (comment_id, user_idx) VALUES "
          "(%d, (SELECT user_idx FROM users WHERE id = '%s'))",
          comment_id, esc_user_id);
  if (mysql_query(conn, query)) {
    return 0;
  }
  return 1;
}

int get_comment_likes_count(MYSQL *conn, int comment_id) {
  char query[256];
  sprintf(query, "SELECT COUNT(*) FROM comment_likes WHERE comment_id = %d",
          comment_id);
  if (mysql_query(conn, query)) {
    return 0;
  }
  MYSQL_RES *res = mysql_store_result(conn);
  if (res == NULL)
    return 0;
  MYSQL_ROW row = mysql_fetch_row(res);
  int count = row ? atoi(row[0]) : 0;
  mysql_free_result(res);
  return count;
}

int has_user_liked_comment(MYSQL *conn, int comment_id, const char *user_id) {
  char esc_user_id[100];
  mysql_real_escape_string(conn, esc_user_id, user_id, strlen(user_id));

  char query[512];
  sprintf(query,
          "SELECT 1 FROM comment_likes WHERE comment_id = %d AND user_idx = "
          "(SELECT user_idx FROM users WHERE id = '%s')",
          comment_id, esc_user_id);
  if (mysql_query(conn, query))
    return 0;
  MYSQL_RES *res = mysql_store_result(conn);
  int liked = 0;
  if (res) {
    if (mysql_num_rows(res) > 0)
      liked = 1;
    mysql_free_result(res);
  }
  return liked;
}

int like_post(MYSQL *conn, int post_id, const char *logged_id) {
  char query[256];
  sprintf(query,
          "UPDATE posts SET like_count = like_count + 1 WHERE post_id = %d",
          post_id);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "게시글 좋아요 실패: %s\n", mysql_error(conn));
    return 0;
  }
  return 1;
}

int is_user_club_leader(MYSQL *conn, const char *user_id) {
  char query[512];
  char esc_user_id[100];
  mysql_real_escape_string(conn, esc_user_id, user_id, strlen(user_id));

  // Check if they are Admin in users table
  sprintf(query, "SELECT 1 FROM users WHERE id = '%s' AND role = 'Admin'",
          esc_user_id);
  if (mysql_query(conn, query) == 0) {
    MYSQL_RES *res = mysql_store_result(conn);
    if (res) {
      if (mysql_num_rows(res) > 0) {
        mysql_free_result(res);
        return 1; // Admin is allowed to write posts
      }
      mysql_free_result(res);
    }
  }

  // Check if they are Leader in clubmembers table
  sprintf(
      query,
      "SELECT 1 FROM clubmembers cm JOIN users u ON cm.user_idx = u.user_idx "
      "WHERE u.id = '%s' AND cm.role = 'Leader'",
      esc_user_id);
  if (mysql_query(conn, query) == 0) {
    MYSQL_RES *res = mysql_store_result(conn);
    int is_leader = 0;
    if (res) {
      if (mysql_num_rows(res) > 0)
        is_leader = 1;
      mysql_free_result(res);
    }
    return is_leader;
  }
  return 0;
}

int delete_post(MYSQL *conn, int post_id, const char *user_id) {
  char query[256];
  sprintf(query,
          "DELETE FROM posts WHERE post_id = %d AND user_idx = (SELECT "
          "user_idx FROM users WHERE id = '%s')",
          post_id, user_id);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "삭제 실패: %s\n", mysql_error(conn));
    return 0;
  }
  return (int)mysql_affected_rows(conn) > 0 ? 1 : 0;
}

void display_my_profile(MYSQL *conn, const char *logged_id) {
  char query[1024];
  char esc_id[100];
  mysql_real_escape_string(conn, esc_id, logged_id, strlen(logged_id));

  printf("\n=======================================\n");
  printf("         내 프로필 (마이페이지)\n");
  printf("=======================================\n");

  sprintf(query,
          "SELECT user_idx, name, student_id, college_code, major_code "
          "FROM users WHERE id = '%s'",
          esc_id);

  if (mysql_query(conn, query)) {
    printf("정보 조회 실패: %s\n", mysql_error(conn));
    return;
  }

  MYSQL_RES *res_user = mysql_store_result(conn);
  if (!res_user || mysql_num_rows(res_user) == 0) {
    printf("유저 정보를 찾을 수 없습니다.\n");
    if (res_user)
      mysql_free_result(res_user);
    return;
  }

  MYSQL_ROW row_user = mysql_fetch_row(res_user);
  int user_idx = atoi(row_user[0]);
  int u_col = atoi(row_user[3]);
  int u_maj = atoi(row_user[4]);

  printf(" [기본 정보]\n");
  printf(" - 이름: %s\n", row_user[1]);
  printf(" - 학번: %s\n", row_user[2]);
  printf(" - 전공: %s\n", get_major_name(u_col, u_maj));
  mysql_free_result(res_user);

  printf("\n [가입된 동아리 목록]\n");

  sprintf(query,
          "SELECT c.club_name, cm.role, cm.joined_at "
          "FROM clubmembers cm "
          "JOIN clubs c ON cm.club_id = c.club_id "
          "WHERE cm.user_idx = %d "
          "ORDER BY cm.joined_at DESC",
          user_idx);

  if (mysql_query(conn, query)) {
    printf("동아리 목록 조회 실패: %s\n", mysql_error(conn));
    return;
  }

  MYSQL_RES *res_clubs = mysql_store_result(conn);
  if (!res_clubs || mysql_num_rows(res_clubs) == 0) {
    printf(" 가입된 동아리가 없습니다.\n");
  } else {
    MYSQL_ROW row_club;
    while ((row_club = mysql_fetch_row(res_clubs))) {
      const char *club_name = row_club[0];
      const char *role_str = row_club[1];
      const char *joined_at = row_club[2];

      const char *display_role = (strcmp(role_str, "Leader") == 0)
                                     ? "[OWNER(동아리장)]"
                                     : "[MEMBER(일반회원)]";

      printf(" - %s %s (가입일: %s)\n", club_name, display_role, joined_at);
    }
  }
  if (res_clubs)
    mysql_free_result(res_clubs);
  printf("=======================================\n");
}

int increment_bad_word_count(MYSQL *conn, const char *user_id) {
  if (!user_id || strlen(user_id) == 0)
    return 0;

  char esc_id[100];
  mysql_real_escape_string(conn, esc_id, user_id, strlen(user_id));

  char query[512];
  sprintf(
      query,
      "UPDATE users SET bad_word_count = bad_word_count + 1 WHERE id = '%s'",
      esc_id);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "비속어 카운트 증가 실패: %s\n", mysql_error(conn));
    return 0;
  }

  sprintf(query, "SELECT bad_word_count FROM users WHERE id = '%s'", esc_id);
  if (mysql_query(conn, query) == 0) {
    MYSQL_RES *res = mysql_store_result(conn);
    if (res) {
      MYSQL_ROW row = mysql_fetch_row(res);
      if (row) {
        int count = atoi(row[0]);
        mysql_free_result(res);
        if (count >= 5) {
          sprintf(query, "UPDATE users SET is_suspended = 1 WHERE id = '%s'",
                  esc_id);
          mysql_query(conn, query);
          printf("\n==========================================================="
                 "==========\n");
          printf(" ❌ [계정 정지] 비속어 누적 5회 도달로 인해 계정이 즉시 "
                 "정지됩니다.\n");
          printf("============================================================="
                 "========\n");
          return 1;
        } else {
          printf("⚠️ 비속어 누적 경고: 현재 누적 %d회 / 5회 (5회 도달 시 계정이 "
                 "즉시 정지됩니다.)\n",
                 count);
        }
      } else {
        mysql_free_result(res);
      }
    }
  }
  return 0;
}

// ─────────────────────────────────────────────────
// 신규 메시지 기능 (공지 및 1:1 쪽지)
// ─────────────────────────────────────────────────

int send_club_announcement(MYSQL *conn, int club_id, const char *leader_id,
                           const char *content) {
  char query[2048];
  char esc_leader_id[100];
  mysql_real_escape_string(conn, esc_leader_id, leader_id, strlen(leader_id));

  // 1. 권한 검증: leader_id가 club_id의 OWNER 인지 확인
  sprintf(query,
          "SELECT cm.role FROM clubmembers cm JOIN users u ON cm.user_idx = "
          "u.user_idx WHERE cm.club_id = %d AND u.id = '%s'",
          club_id, esc_leader_id);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "권한 조회 실패: %s\n", mysql_error(conn));
    return 0;
  }
  MYSQL_RES *res = mysql_store_result(conn);
  int is_owner = 0;
  if (res && mysql_num_rows(res) > 0) {
    MYSQL_ROW row = mysql_fetch_row(res);
    if (strcmp(row[0], "Leader") == 0) {
      is_owner = 1;
    }
  }
  if (res)
    mysql_free_result(res);

  if (!is_owner) {
    printf("⚠️ 권한이 없습니다. 동아리장만 공지를 발송할 수 있습니다.\n");
    return 0;
  }

  char esc_content[1024];
  mysql_real_escape_string(conn, esc_content, content, strlen(content));

  // 동아리 이름 조회
  char club_name[100] = "System";
  sprintf(query, "SELECT club_name FROM clubs WHERE club_id = %d", club_id);
  if (mysql_query(conn, query) == 0) {
    res = mysql_store_result(conn);
    if (res) {
      MYSQL_ROW row = mysql_fetch_row(res);
      if (row)
        strcpy(club_name, row[0]);
      mysql_free_result(res);
    }
  }
  char esc_club_name[200];
  mysql_real_escape_string(conn, esc_club_name, club_name, strlen(club_name));

  // 동아리 소속 회원 조회 (승인된 회원 모두)
  sprintf(query, "SELECT user_idx FROM clubmembers WHERE club_id = %d",
          club_id);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "회원 조회 실패: %s\n", mysql_error(conn));
    return 0;
  }

  res = mysql_store_result(conn);
  if (!res)
    return 0;

  int success_count = 0;
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res))) {
    int target_idx = atoi(row[0]);
    char insert_query[1024];
    // msg_type = 0 (ANNOUNCEMENT)
    sprintf(insert_query,
            "INSERT INTO messages (receiver_idx, contented_at, msg_type, "
            "sender_info) "
            "VALUES (%d, '%s', 0, '%s')",
            target_idx, esc_content, esc_club_name);
    if (mysql_query(conn, insert_query) == 0) {
      success_count++;
    }
  }
  mysql_free_result(res);

  printf("✅ 총 %d명의 회원에게 단체 공지를 발송했습니다.\n", success_count);
  return 1;
}

int send_direct_message(MYSQL *conn, const char *sender_id,
                        const char *receiver_id, const char *content) {
  char esc_receiver_id[100];
  mysql_real_escape_string(conn, esc_receiver_id, receiver_id,
                           strlen(receiver_id));

  // 1. users 테이블에서 receiver_id 존재 여부 확인
  char query[512];
  sprintf(query, "SELECT user_idx FROM users WHERE id = '%s'", esc_receiver_id);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "수신자 조회 실패: %s\n", mysql_error(conn));
    return 0;
  }

  MYSQL_RES *res = mysql_store_result(conn);
  if (!res || mysql_num_rows(res) == 0) {
    printf("❌ 존재하지 않는 사용자입니다.\n");
    if (res)
      mysql_free_result(res);
    return 0;
  }
  MYSQL_ROW row = mysql_fetch_row(res);
  int target_idx = atoi(row[0]);
  mysql_free_result(res);

  char esc_content[1024];
  mysql_real_escape_string(conn, esc_content, content, strlen(content));

  char esc_sender_id[100];
  mysql_real_escape_string(conn, esc_sender_id, sender_id, strlen(sender_id));

  // 2. messages 테이블에 INSERT (msg_type = 1 (DM))
  char insert_query[1024];
  sprintf(insert_query,
          "INSERT INTO messages (receiver_idx, contented_at, msg_type, "
          "sender_info) "
          "VALUES (%d, '%s', 1, '%s')",
          target_idx, esc_content, esc_sender_id);

  if (mysql_query(conn, insert_query)) {
    fprintf(stderr, "❌ 메시지 전송 실패: %s\n", mysql_error(conn));
    return 0;
  }

  printf("✅ 쪽지를 성공적으로 전송했습니다.\n");
  return 1;
}

int is_currently_suspended(MYSQL *conn, const char *user_id) {
  if (!user_id || strlen(user_id) == 0)
    return 0;

  char esc_id[100];
  mysql_real_escape_string(conn, esc_id, user_id, strlen(user_id));

  char query[512];
  sprintf(query, "SELECT is_suspended FROM users WHERE id = '%s'", esc_id);
  if (mysql_query(conn, query) == 0) {
    MYSQL_RES *res = mysql_store_result(conn);
    if (res) {
      MYSQL_ROW row = mysql_fetch_row(res);
      if (row) {
        int suspended = atoi(row[0]);
        mysql_free_result(res);
        return suspended;
      }
      mysql_free_result(res);
    }
  }
  return 0;
}
