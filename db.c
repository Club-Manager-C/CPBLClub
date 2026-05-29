#include "db.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

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
                     "major VARCHAR(50) NOT NULL, "
                     "name VARCHAR(20) NOT NULL, "
                     "phone VARCHAR(15) NOT NULL UNIQUE, "
                     "role ENUM('User', 'Admin') NOT NULL DEFAULT 'User', "
                     "is_approved TINYINT(1) DEFAULT 0, "
                     "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");

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
      "leader_idx INT NULL, "
      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
      "FOREIGN KEY (category_id) REFERENCES club_categories (category_id), "
      "FOREIGN KEY (leader_idx) REFERENCES users (user_idx) ON DELETE SET "
      "NULL)");

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

  // 기존 버전에 parent_comment_id가 없을 경우를 대비한 마이그레이션 코드 (이미 존재하면 쿼리 실패하며 무시됨)
  mysql_query(*conn, "ALTER TABLE comments ADD COLUMN parent_comment_id INT DEFAULT NULL");
  mysql_query(*conn, "ALTER TABLE comments ADD CONSTRAINT fk_parent_comment FOREIGN KEY (parent_comment_id) REFERENCES comments(comment_id) ON DELETE CASCADE");

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

  mysql_query(*conn, "INSERT IGNORE INTO users (user_idx, id, pw, nickname, "
                     "student_id, major, name, phone, role, is_approved) "
                     "VALUES (1, 'admin', 'admin1234', '관리자', '00000000', "
                     "'컴퓨터공학', '관리자', '010-0000-0000', 'Admin', 1)");

  mysql_query(*conn, "INSERT IGNORE INTO clubs (club_id, club_name, category_id, leader_idx, status, description) "
                     "VALUES (1, '기본 동아리', 1, 1, '승인', '시스템 기본 설정 동아리입니다.')");

  mysql_query(*conn, "INSERT IGNORE INTO boardtype (type_id, board_name) VALUES (1, '동아리 홍보'), (2, '전공 동아리'), (3, '동아리 공지사항')");
}

int check_login(MYSQL *conn, const char *id, const char *pw) {
  char query[256];
  sprintf(query, "SELECT pw FROM users WHERE id = '%s'", id);

  if (mysql_query(conn, query)) {
    fprintf(stderr, "쿼리 실패: %s\n", mysql_error(conn));
    return 0; // 실패
  }

  MYSQL_RES *res = mysql_store_result(conn);
  if (res != NULL && mysql_num_rows(res) > 0) {
    MYSQL_ROW row = mysql_fetch_row(res);
    if (strcmp(row[0], pw) == 0) {
      mysql_free_result(res);
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
                  const char *major, const char *phone) {
  char query[768];
  sprintf(
      query,
      "INSERT INTO users (id, pw, nickname, student_id, name, major, phone) "
      "VALUES ('%s', '%s', '%s', '%lld', '%s', '%s', '%s')",
      id, pw, nickname, student_id, name, major, phone);

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
          "SELECT p.post_id, p.title, u.nickname, p.created_at "
          "FROM posts p JOIN users u ON p.user_idx = u.user_idx "
          "WHERE p.type_id = %d ORDER BY p.created_at DESC",
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

  printf("%-6s %-30s %-15s %-20s\n", "ID", "제목", "작성자", "작성일");
  printf("---------------------------------------------------------------------"
         "-----\n");
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res))) {
    printf("[%-4s] %-30s %-15s %s\n", row[0], row[1], row[2], row[3]);
  }
  mysql_free_result(res);
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

int insert_post(MYSQL *conn, const char *user_id, int category_id,
                const char *title, const char *content) {
  char esc_title[500], esc_content[2500], esc_user_id[100];
  mysql_real_escape_string(conn, esc_title, title, strlen(title));
  mysql_real_escape_string(conn, esc_content, content, strlen(content));
  mysql_real_escape_string(conn, esc_user_id, user_id, strlen(user_id));

  char query[4000];
  sprintf(
      query,
      "INSERT INTO posts (club_id, user_idx, type_id, title, content) VALUES "
      "(1, (SELECT user_idx FROM users WHERE id = '%s'), %d, '%s', '%s')",
      esc_user_id, category_id, esc_title, esc_content);
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

// ─────────────────────────────────────────────────
// 마이페이지 상세 조회 및 계정 탈퇴
// ─────────────────────────────────────────────────

void display_my_profile(MYSQL *conn, const char *logged_id) {
    char query[1024];
    char esc_id[100];
    mysql_real_escape_string(conn, esc_id, logged_id, strlen(logged_id));

    printf("\n=======================================\n");
    printf("         내 프로필 (마이페이지)\n");
    printf("=======================================\n");

    sprintf(query, 
        "SELECT user_idx, name, student_id, major "
        "FROM users WHERE id = '%s'", esc_id);

    if (mysql_query(conn, query)) {
        printf("정보 조회 실패: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *res_user = mysql_store_result(conn);
    if (!res_user || mysql_num_rows(res_user) == 0) {
        printf("유저 정보를 찾을 수 없습니다.\n");
        if (res_user) mysql_free_result(res_user);
        return;
    }

    MYSQL_ROW row_user = mysql_fetch_row(res_user);
    int user_idx = atoi(row_user[0]);
    
    printf(" [기본 정보]\n");
    printf(" - 이름: %s\n", row_user[1]);
    printf(" - 학번: %s\n", row_user[2]);
    printf(" - 전공: %s\n", row_user[3]);
    mysql_free_result(res_user);

    printf("\n [가입된 동아리 목록]\n");

    sprintf(query,
        "SELECT c.club_name, cm.role, cm.joined_at "
        "FROM clubmembers cm "
        "JOIN clubs c ON cm.club_id = c.club_id "
        "WHERE cm.user_idx = %d "
        "ORDER BY cm.joined_at DESC", user_idx);

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
            const char* club_name = row_club[0];
            const char* role_str = row_club[1];
            const char* joined_at = row_club[2];

            const char* display_role = (strcmp(role_str, "Leader") == 0) 
                                        ? "[OWNER(동아리장)]" 
                                        : "[MEMBER(일반회원)]";

            printf(" - %s %s (가입일: %s)\n", club_name, display_role, joined_at);
        }
    }
    if (res_clubs) mysql_free_result(res_clubs);
    printf("=======================================\n");
}

int delete_user_account(MYSQL *conn, const char *logged_id) {
    char query[512];
    char esc_id[100];
    mysql_real_escape_string(conn, esc_id, logged_id, strlen(logged_id));

    sprintf(query, "SELECT user_idx FROM users WHERE id = '%s'", esc_id);
    if (mysql_query(conn, query)) return 0;
    
    MYSQL_RES *res = mysql_store_result(conn);
    if (!res || mysql_num_rows(res) == 0) {
        if (res) mysql_free_result(res);
        return 0;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    int user_idx = atoi(row[0]);
    mysql_free_result(res);

    sprintf(query, 
        "SELECT c.club_name FROM clubmembers cm "
        "JOIN clubs c ON cm.club_id = c.club_id "
        "WHERE cm.user_idx = %d AND cm.role = 'Leader'", user_idx);

    if (mysql_query(conn, query)) return 0;

    MYSQL_RES *leader_res = mysql_store_result(conn);
    if (leader_res && mysql_num_rows(leader_res) > 0) {
        MYSQL_ROW club_row = mysql_fetch_row(leader_res);
        printf("\n[탈퇴 불가 안내]\n");
        printf("회원님은 현재 '%s' 동아리의 동아리장(OWNER)입니다.\n", club_row[0]);
        printf("동아리장은 계정을 바로 탈퇴할 수 없습니다.\n");
        printf("먼저 동아리장 위임을 진행하시거나, 동아리 폐쇄를 요청해 주세요.\n");
        mysql_free_result(leader_res);
        return 0;
    }
    if (leader_res) mysql_free_result(leader_res);

    printf("정말 탈퇴하시겠습니까? (복구 불가) [Y/N]: ");
    char check;
    scanf(" %c", &check);
    
    if (check == 'Y' || check == 'y') {
        sprintf(query, "DELETE FROM users WHERE user_idx = %d", user_idx);
        
        if (mysql_query(conn, query)) {
            printf("[오류] 계정 삭제 중 문제가 발생했습니다: %s\n", mysql_error(conn));
            return 0;
        }

        printf("\n회원 탈퇴가 완료되었습니다. 관련된 모든 내역이 삭제되었습니다.\n");
        printf("이용해 주셔서 감사합니다.\n");
        return 1;
    } else {
        printf("탈퇴를 취소합니다.\n");
        return 0;
    }
}

// ─────────────────────────────────────────────────
// 내 동아리 공간 및 공지사항
// ─────────────────────────────────────────────────

int get_user_joined_clubs(MYSQL *conn, const char *logged_id, int *club_ids_out, char club_names_out[][100], int *roles_out) {
    char query[512];
    char esc_id[100];
    mysql_real_escape_string(conn, esc_id, logged_id, strlen(logged_id));

    sprintf(query,
        "SELECT c.club_id, c.club_name, cm.role "
        "FROM clubmembers cm "
        "JOIN clubs c ON cm.club_id = c.club_id "
        "WHERE cm.user_idx = (SELECT user_idx FROM users WHERE id = '%s')", esc_id);

    if (mysql_query(conn, query)) {
        printf("동아리 목록 조회 실패: %s\n", mysql_error(conn));
        return 0;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (!res) return 0;

    int count = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        club_ids_out[count] = atoi(row[0]);
        strcpy(club_names_out[count], row[1]);
        roles_out[count] = (strcmp(row[2], "Leader") == 0) ? 1 : 0; // 1 = OWNER, 0 = MEMBER
        count++;
    }
    mysql_free_result(res);
    return count;
}

void get_club_notices(MYSQL *conn, int club_id) {
    char query[512];
    // type_id = 3 (동아리 내부 공지사항)
    sprintf(query, 
        "SELECT p.post_id, p.title, u.nickname, p.created_at "
        "FROM posts p "
        "JOIN users u ON p.user_idx = u.user_idx "
        "WHERE p.club_id = %d AND p.type_id = 3 "
        "ORDER BY p.created_at DESC", club_id);

    if (mysql_query(conn, query)) {
        printf("공지사항 조회 에러: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (!res) return;

    if (mysql_num_rows(res) == 0) {
        printf("등록된 공지사항이 없습니다.\n");
        mysql_free_result(res);
        return;
    }

    printf("%-5s | %-30s | %-15s | %-20s\n", "번호", "제목", "작성자", "작성일");
    printf("-----------------------------------------------------------------------\n");
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        printf("%-5s | %-30s | %-15s | %-20s\n", row[0], row[1], row[2], row[3]);
    }
    mysql_free_result(res);
}

int insert_club_notice(MYSQL *conn, int club_id, const char *user_id, const char *title, const char *content) {
    char esc_title[500], esc_content[2500], esc_user_id[100];
    mysql_real_escape_string(conn, esc_title, title, strlen(title));
    mysql_real_escape_string(conn, esc_content, content, strlen(content));
    mysql_real_escape_string(conn, esc_user_id, user_id, strlen(user_id));
    
    char query[4000];
    // type_id = 3 (동아리 내부 공지사항)
    sprintf(query, "INSERT INTO posts (club_id, user_idx, type_id, title, content) VALUES "
                   "(%d, (SELECT user_idx FROM users WHERE id = '%s'), 3, '%s', '%s')",
            club_id, esc_user_id, esc_title, esc_content);
            
    if (mysql_query(conn, query)) {
        fprintf(stderr, "공지사항 작성 실패: %s\n", mysql_error(conn));
        return 0;
    }
    return 1;
}

// ─────────────────────────────────────────────────
// 동아리장 관리 기능 (강퇴, 위임, 정보 수정)
// ─────────────────────────────────────────────────

int verify_club_owner(MYSQL *conn, int club_id, const char *logged_id) {
    char query[512];
    MYSQL_RES *res;
    char esc_logged_id[100];
    mysql_real_escape_string(conn, esc_logged_id, logged_id, strlen(logged_id));

    sprintf(query, 
            "SELECT 1 FROM clubs c "
            "JOIN users u ON c.leader_idx = u.user_idx "
            "WHERE c.club_id = %d AND u.id = '%s' AND c.status = '승인'", 
            club_id, esc_logged_id);

    if (mysql_query(conn, query)) return 0;
    res = mysql_store_result(conn);
    if (!res) return 0;

    int is_owner = (mysql_num_rows(res) > 0) ? 1 : 0;
    mysql_free_result(res);

    if (is_owner) {
        sprintf(query, 
                "SELECT 1 FROM clubmembers cm "
                "JOIN users u ON cm.user_idx = u.user_idx "
                "WHERE cm.club_id = %d AND u.id = '%s' AND cm.role = 'Leader'", 
                club_id, esc_logged_id);
        
        if (mysql_query(conn, query) == 0) {
            MYSQL_RES *res_member = mysql_store_result(conn);
            if (res_member) {
                is_owner = (mysql_num_rows(res_member) > 0) ? 1 : 0;
                mysql_free_result(res_member);
            }
        }
    }
    return is_owner;
}

int kick_club_member(MYSQL *conn, int club_id, const char *owner_id, const char *target_student_id) {
    if (!verify_club_owner(conn, club_id, owner_id)) {
        printf("⚠️ 권한이 없습니다. 동아리장만 회원 강퇴가 가능합니다.\n");
        return 0;
    }

    char esc_student_id[20];
    mysql_real_escape_string(conn, esc_student_id, target_student_id, strlen(target_student_id));

    char query[512];
    sprintf(query, "SELECT student_id FROM users WHERE id = '%s'", owner_id);
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *res = mysql_store_result(conn);
        if (res) {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row && strcmp(row[0], esc_student_id) == 0) {
                printf("⚠️ 동아리장 자기 자신은 강퇴할 수 없습니다.\n");
                mysql_free_result(res);
                return 0;
            }
            mysql_free_result(res);
        }
    }

    sprintf(query, 
            "SELECT cm.user_idx FROM clubmembers cm "
            "JOIN users u ON cm.user_idx = u.user_idx "
            "WHERE cm.club_id = %d AND u.student_id = '%s'", 
            club_id, esc_student_id);

    if (mysql_query(conn, query)) return 0;
    MYSQL_RES *res_target = mysql_store_result(conn);
    if (res_target == NULL || mysql_num_rows(res_target) == 0) {
        printf("⚠️ 존재하지 않거나 해당 동아리에 소속되어 있지 않은 회원입니다.\n");
        if (res_target) mysql_free_result(res_target);
        return 0;
    }
    MYSQL_ROW target_row = mysql_fetch_row(res_target);
    int target_user_idx = atoi(target_row[0]);
    mysql_free_result(res_target);

    sprintf(query, "DELETE FROM clubmembers WHERE club_id = %d AND user_idx = %d", club_id, target_user_idx);
    if (mysql_query(conn, query)) {
        printf("⚠️ 강퇴 처리 실패: %s\n", mysql_error(conn));
        return 0;
    }

    printf("✅ 학번 %s 회원을 동아리에서 강퇴 완료하였습니다.\n", target_student_id);
    return 1;
}

int transfer_club_ownership(MYSQL *conn, int club_id, const char *current_owner_id, const char *target_student_id) {
    if (!verify_club_owner(conn, club_id, current_owner_id)) {
        printf("⚠️ 위임 권한이 없습니다. 현재 동아리장만 위임이 가능합니다.\n");
        return 0;
    }

    char query[512];
    char esc_student_id[20];
    mysql_real_escape_string(conn, esc_student_id, target_student_id, strlen(target_student_id));

    sprintf(query, "SELECT student_id FROM users WHERE id = '%s'", current_owner_id);
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *res = mysql_store_result(conn);
        if (res) {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row && strcmp(row[0], esc_student_id) == 0) {
                printf("⚠️ 자기 자신에게는 동아리장 권한을 위임할 수 없습니다.\n");
                mysql_free_result(res);
                return 0;
            }
            mysql_free_result(res);
        }
    }

    sprintf(query, 
            "SELECT cm.user_idx FROM clubmembers cm "
            "JOIN users u ON cm.user_idx = u.user_idx "
            "WHERE cm.club_id = %d AND u.student_id = '%s' AND cm.role = 'Member'", 
            club_id, esc_student_id);

    if (mysql_query(conn, query)) return 0;
    MYSQL_RES *res_target = mysql_store_result(conn);
    if (res_target == NULL || mysql_num_rows(res_target) == 0) {
        printf("⚠️ 해당 동아리에 속한 일반 회원(Member) 중 학번 %s인 회원이 없습니다.\n", target_student_id);
        if (res_target) mysql_free_result(res_target);
        return 0;
    }
    MYSQL_ROW target_row = mysql_fetch_row(res_target);
    int new_leader_idx = atoi(target_row[0]);
    mysql_free_result(res_target);

    int old_leader_idx = -1;
    sprintf(query, "SELECT user_idx FROM users WHERE id = '%s'", current_owner_id);
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *res_old = mysql_store_result(conn);
        if (res_old) {
            MYSQL_ROW row = mysql_fetch_row(res_old);
            if (row) old_leader_idx = atoi(row[0]);
            mysql_free_result(res_old);
        }
    }

    if (old_leader_idx == -1) return 0;

    mysql_query(conn, "START TRANSACTION");

    sprintf(query, "UPDATE clubs SET leader_idx = %d WHERE club_id = %d", new_leader_idx, club_id);
    if (mysql_query(conn, query)) {
        mysql_query(conn, "ROLLBACK");
        printf("⚠️ 대표자 변경 실패.\n");
        return 0;
    }

    sprintf(query, "UPDATE clubmembers SET role = 'Leader' WHERE club_id = %d AND user_idx = %d", club_id, new_leader_idx);
    if (mysql_query(conn, query)) {
        mysql_query(conn, "ROLLBACK");
        return 0;
    }

    sprintf(query, "UPDATE clubmembers SET role = 'Member' WHERE club_id = %d AND user_idx = %d", club_id, old_leader_idx);
    if (mysql_query(conn, query)) {
        mysql_query(conn, "ROLLBACK");
        return 0;
    }

    mysql_query(conn, "COMMIT");
    printf("✅ 동아리장 권한 위임이 완료되었습니다. (신규 동아리장 학번: %s)\n", target_student_id);
    return 1;
}

static int is_all_whitespace(const char *str) {
    if (!str) return 1;
    while (*str) {
        if (!isspace((unsigned char)*str)) return 0;
        str++;
    }
    return 1;
}

int update_club_info(MYSQL *conn, int club_id, const char *owner_id, const char *new_name, const char *new_desc) {
    if (!verify_club_owner(conn, club_id, owner_id)) {
        printf("⚠️ 권한이 없습니다. 동아리장만 정보 수정이 가능합니다.\n");
        return 0;
    }

    if (!new_name || is_all_whitespace(new_name)) {
        printf("⚠️ 동아리명은 필수 항목입니다.\n");
        return 0;
    }
    if (!new_desc || is_all_whitespace(new_desc)) {
        printf("⚠️ 소개글은 필수 항목입니다.\n");
        return 0;
    }

    if (strlen(new_name) > 50) {
        printf("⚠️ 동아리 이름이 너무 깁니다. (최대 50바이트)\n");
        return 0;
    }
    if (strlen(new_desc) > 300) {
        printf("⚠️ 동아리 소개글이 너무 깁니다. (최대 300바이트)\n");
        return 0;
    }

    char esc_name[200];
    char esc_desc[1000];
    mysql_real_escape_string(conn, esc_name, new_name, strlen(new_name));
    mysql_real_escape_string(conn, esc_desc, new_desc, strlen(new_desc));

    char query[1500];
    sprintf(query, "UPDATE clubs SET club_name = '%s', description = '%s' WHERE club_id = %d", esc_name, esc_desc, club_id);

    if (mysql_query(conn, query)) {
        printf("⚠️ 동아리 정보 변경 실패: %s\n", mysql_error(conn));
        return 0;
    }

    printf("✅ 동아리 정보(이름, 소개글)가 수정되었습니다.\n");
    return 1;
}

