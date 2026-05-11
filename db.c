#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *DB_HOST = "localhost";
const char *DB_USER = "root";
const char *DB_PW = "rds12003!";
const char *DB_NAME = "cpbl_db";

void init_db(MYSQL **conn) {
  *conn = mysql_init(NULL);
  if (*conn == NULL) {
    fprintf(stderr, "mysql_init() 실패\n");
    exit(1);
  }

  // DB 서버 접속
  if (mysql_real_connect(*conn, DB_HOST, DB_USER, DB_PW, NULL, 3306, NULL, 0) == NULL) {
    fprintf(stderr, "MySQL 연결 실패: %s\n", mysql_error(*conn));
    exit(1);
  }

  // 사용할 데이터베이스 생성 및 선택
  mysql_query(*conn, "CREATE DATABASE IF NOT EXISTS cpbl_db");
  mysql_select_db(*conn, DB_NAME);

  // 사용자 테이블 생성
  const char *create_table_query = "CREATE TABLE IF NOT EXISTS users ("
                                   "id VARCHAR(50) PRIMARY KEY, "
                                   "pw VARCHAR(50) NOT NULL, "
                                   "nickname VARCHAR(50) NOT NULL, "
                                   "student_id BIGINT UNIQUE NOT NULL, "
                                   "name VARCHAR(50) NOT NULL, "
                                   "major VARCHAR(50) NOT NULL, "
                                   "phone VARCHAR(20) NOT NULL)";
  if (mysql_query(*conn, create_table_query)) {
    fprintf(stderr, "테이블 생성 실패: %s\n", mysql_error(*conn));
  }
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
  if (res != NULL) mysql_free_result(res);
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
    if (res != NULL) mysql_free_result(res);
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
    if (res != NULL) mysql_free_result(res);
  }
  return 0; // 중복 안됨 (사용 가능)
}

int check_student_id_duplicate(MYSQL *conn, long long student_id) {
  char query[256];
  sprintf(query, "SELECT student_id FROM users WHERE student_id = %lld", student_id);
  if (mysql_query(conn, query) == 0) {
    MYSQL_RES *res = mysql_store_result(conn);
    if (res != NULL && mysql_num_rows(res) > 0) {
      mysql_free_result(res);
      return 1; // 중복됨
    }
    if (res != NULL) mysql_free_result(res);
  }
  return 0; // 중복 안됨
}

int register_user(MYSQL *conn, const char *id, const char *pw,
                  const char *nickname, long long student_id,
                  const char *name, const char *major, const char *phone) {
  char query[768];
  sprintf(query,
    "INSERT INTO users (id, pw, nickname, student_id, name, major, phone) "
    "VALUES ('%s', '%s', '%s', %lld, '%s', '%s', '%s')",
    id, pw, nickname, student_id, name, major, phone);

  if (mysql_query(conn, query)) {
    fprintf(stderr, "회원가입 실패: %s\n", mysql_error(conn));
    return 0; // 실패
  }
  return 1; // 성공
}

void close_db(MYSQL *conn) {
  if (conn) {
    mysql_close(conn);
  }
}
