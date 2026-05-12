#ifndef AUTH_H
#define AUTH_H

#include "db.h"
#include <ctype.h>

// ── 계정 구조체 ──────────────────────────────────
typedef struct {
  char id[50];
  char pw[50];
  char nickname[50];
  long long student_id; // 10자리 학번
  char name[50];        // 이름
  char major[50];       // 전공
  char phone[20];       // 전화번호
} Account;

// 비밀번호 형식 검사: 영문+숫자+허용 특수문자(!?$)
// 반환값: 1 = 형식 OK, 0 = 허용되지 않는 문자 포함
int validate_format(const char *str, int *has_alpha, int *has_digit);

// 로그인 화면: 성공 시 logged_id에 ID를 담아 반환 (1), 실패 시 0
int login_screen(MYSQL *conn, char *logged_id);

// 회원가입 화면
void register_screen(MYSQL *conn);

#endif // AUTH_H
