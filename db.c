#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
  bool my_ssl = false;
  mysql_options(*conn, MYSQL_OPT_SSL_MODE, &my_ssl);

  if (mysql_real_connect(*conn, DB_HOST, DB_USER, DB_PW, NULL, 3306, NULL, 0) == NULL) {
    fprintf(stderr, "MySQL 연결 실패: %s\n", mysql_error(*conn));
    exit(1);
  }

  // 한글 깨짐 방지: 클라이언트 연결 인코딩을 utf8로 설정
  mysql_set_character_set(*conn, "utf8");

  mysql_query(*conn, "CREATE DATABASE IF NOT EXISTS cpbl_db");
  mysql_select_db(*conn, DB_NAME);

  // 1. 사용자 테이블
  mysql_query(*conn, "CREATE TABLE IF NOT EXISTS users ("
                     "id VARCHAR(50) PRIMARY KEY, "
                     "pw VARCHAR(50) NOT NULL, "
                     "nickname VARCHAR(50) NOT NULL, "
                     "student_id BIGINT UNIQUE NOT NULL, "
                     "name VARCHAR(50) NOT NULL, "
                     "major VARCHAR(50) NOT NULL, "
                     "phone VARCHAR(20) NOT NULL)");

  // 2. 카테고리 테이블 (posts보다 먼저 생성해야 외래키 연결 가능)
  mysql_query(*conn, "CREATE TABLE IF NOT EXISTS categories ("
                     "category_id INT PRIMARY KEY, "
                     "name VARCHAR(50) NOT NULL)");
  mysql_query(*conn, "INSERT IGNORE INTO categories (category_id, name) VALUES (1, '동아리 홍보'), (2, '전공 동아리')");

  // 3. 게시글 테이블 (처음부터 category_id 포함)
  mysql_query(*conn, "CREATE TABLE IF NOT EXISTS posts ("
                     "post_id INT AUTO_INCREMENT PRIMARY KEY, "
                     "user_id VARCHAR(50) NOT NULL, "
                     "category_id INT DEFAULT 1, "
                     "title VARCHAR(200) NOT NULL, "
                     "content TEXT NOT NULL, "
                     "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
                     "FOREIGN KEY (user_id) REFERENCES users(id), "
                     "FOREIGN KEY (category_id) REFERENCES categories(category_id))");

  // 4. 댓글 테이블
  mysql_query(*conn, "CREATE TABLE IF NOT EXISTS comments ("
                     "comment_id INT AUTO_INCREMENT PRIMARY KEY, "
                     "post_id INT NOT NULL, "
                     "user_id VARCHAR(50) NOT NULL, "
                     "content TEXT NOT NULL, "
                     "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
                     "FOREIGN KEY (user_id) REFERENCES users(id), "
                     "FOREIGN KEY (post_id) REFERENCES posts(post_id))");

  // 5. 시간표 테이블
  mysql_query(*conn, "CREATE TABLE IF NOT EXISTS schedules ("
                     "schedule_id INT AUTO_INCREMENT PRIMARY KEY, "
                     "user_id VARCHAR(50) NOT NULL, "
                     "day_of_week VARCHAR(10) NOT NULL, "
                     "start_time VARCHAR(10) NOT NULL, "
                     "end_time VARCHAR(10) NOT NULL, "
                     "subject VARCHAR(100) NOT NULL, "
                     "location VARCHAR(100) NOT NULL, "
                     "FOREIGN KEY (user_id) REFERENCES users(id))");

  // 6. 동아리 분류 카테고리
  mysql_query(*conn, "CREATE TABLE IF NOT EXISTS club_categories ("
                     "category_id INT AUTO_INCREMENT PRIMARY KEY, "
                     "category_name VARCHAR(50) NOT NULL)");
  mysql_query(*conn, "INSERT IGNORE INTO club_categories (category_id, category_name) VALUES "
                     "(1, '전공'), (2, '밴드'), (3, '댄스'), (4, '봉사'), (5, '취미'), (6, '운동')");

  // 7. 동아리 테이블
  mysql_query(*conn, "CREATE TABLE IF NOT EXISTS clubs ("
                     "club_id INT AUTO_INCREMENT PRIMARY KEY, "
                     "club_name VARCHAR(100) NOT NULL, "
                     "leader_id VARCHAR(50) NOT NULL, "
                     "category_id INT NOT NULL, "
                     "purpose VARCHAR(300) NOT NULL, "
                     "status VARCHAR(20) DEFAULT '대기', "
                     "reject_reason VARCHAR(255) DEFAULT NULL, "
                     "apply_date VARCHAR(50), "
                     "FOREIGN KEY (leader_id) REFERENCES users(id), "
                     "FOREIGN KEY (category_id) REFERENCES club_categories(category_id))");

  // 8. 알림 메시지 테이블
  mysql_query(*conn, "CREATE TABLE IF NOT EXISTS messages ("
                     "message_id INT AUTO_INCREMENT PRIMARY KEY, "
                     "user_id VARCHAR(50) NOT NULL, "
                     "content TEXT NOT NULL, "
                     "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
                     "FOREIGN KEY (user_id) REFERENCES users(id))");
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
  sprintf(query, "SELECT student_id FROM users WHERE student_id = %lld",
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
      "VALUES ('%s', '%s', '%s', %lld, '%s', '%s', '%s')",
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
          "SELECT post_id, title, created_at FROM posts "
          "WHERE user_id = '%s' ORDER BY created_at DESC",
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
          "WHERE post_id = %d AND user_id = '%s'",
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
          "FROM posts p JOIN users u ON p.user_id = u.id "
          "WHERE p.category_id = %d ORDER BY p.created_at DESC",
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
          "WHERE c.user_id = '%s' ORDER BY c.created_at DESC",
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
          "WHERE comment_id = %d AND user_id = '%s'",
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
          "SELECT schedule_id, day_of_week, start_time, end_time, subject, "
          "location "
          "FROM schedules WHERE user_id = '%s' "
          "ORDER BY FIELD(day_of_week,'월','화','수','목','금','토','일'), "
          "start_time",
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
  printf("%-6s %-5s %-7s %-7s %-20s %-20s\n", "ID", "요일", "시작", "종료",
         "과목명", "강의실");
  printf("------------------------------------------------------------\n");
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res))) {
    printf("[%-4s] %-5s %-7s %-7s %-20s %-20s\n", row[0], row[1], row[2],
           row[3], row[4], row[5]);
  }
  mysql_free_result(res);
}

// 시간표 항목 추가
int add_schedule(MYSQL *conn, const char *user_id, const char *day,
                 const char *start, const char *end, const char *subject,
                 const char *location) {
  char query[512];
  sprintf(query,
          "INSERT INTO schedules "
          "(user_id, day_of_week, start_time, end_time, subject, location) "
          "VALUES ('%s','%s','%s','%s','%s','%s')",
          user_id, day, start, end, subject, location);
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
          "DELETE FROM schedules "
          "WHERE schedule_id = %d AND user_id = '%s'",
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

int insert_message(MYSQL *conn, int receiver_idx, const char *content) {
  char query[1024];
  sprintf(query, "INSERT INTO messages (receiver_idx, content) VALUES (%d, '%s')", receiver_idx, content);
  if (mysql_query(conn, query)) {
    fprintf(stderr, "알림 등록 실패: %s\n", mysql_error(conn));
    return 0;
  }
  return 1;
}

int get_user_idx_by_id(MYSQL *conn, const char *user_id) {
  char query[256];
  sprintf(query, "SELECT user_idx FROM users WHERE user_id = '%s'", user_id);
  if (mysql_query(conn, query)) {
    return -1;
  }
  MYSQL_RES *res = mysql_store_result(conn);
  if (res == NULL || mysql_num_rows(res) == 0) {
    if (res) mysql_free_result(res);
    return -1;
  }
  MYSQL_ROW row = mysql_fetch_row(res);
  int idx = atoi(row[0]);
  mysql_free_result(res);
  return idx;
}
