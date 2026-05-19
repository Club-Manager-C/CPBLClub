#include "auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ─────────────────────────────────────────────
// 비밀번호 형식 검사
// ─────────────────────────────────────────────
int validate_format(const char *str, int *has_alpha, int *has_digit) {
  *has_alpha = 0;
  *has_digit = 0;
  for (int i = 0; str[i] != '\0'; i++) {
    unsigned char c = (unsigned char)str[i];
    if (isalpha(c)) {
      *has_alpha = 1;
    } else if (isdigit(c)) {
      *has_digit = 1;
    } else if (c == '!' || c == '?' || c == '$') {
      // 허용된 특수문자
    } else {
      return 0; // 허용되지 않는 문자
    }
  }
  return 1; // 모든 문자가 유효
}

// ─────────────────────────────────────────────
// 로그인 화면
// ─────────────────────────────────────────────
int login_screen(MYSQL *conn, char *logged_id) {
  int fail_count = 0;
  char pw[50];

  while (fail_count < 5) {
    printf("\n아이디: ");
    scanf("%s", logged_id);
    printf("PW: ");
    scanf("%s", pw);

    if (strcmp(logged_id, "admin") == 0 && strcmp(pw, "admin1234") == 0) {
      return 2; // 총관리자 로그인 성공 코드
    }

    if (check_login(conn, logged_id, pw)) {
      printf("성공했습니다.\n");
      return 1;
    } else {
      fail_count++;
      printf("아이디 혹은 PW를 잘 못 입력했습니다.\n");
      printf("틀린 횟수: %d\n", fail_count);
    }
  }

  printf("5번 틀렸습니다.\n");
  return 0;
}

// ─────────────────────────────────────────────
// 회원가입 화면
// ─────────────────────────────────────────────
void register_screen(MYSQL *conn) {
  Account new_account;

  // ── 아이디 입력 ──
  int valid_id = 0;
  while (!valid_id) {
    printf("\n아이디: ");
    scanf("%s", new_account.id);

    int is_alnum = 1;
    for (int i = 0; new_account.id[i] != '\0'; i++) {
      if (!isalnum((unsigned char)new_account.id[i])) {
        is_alnum = 0;
        break;
      }
    }

    if (!is_alnum) {
      printf("영문자로 입력해주세요\n");
      continue;
    }

    if (check_id_duplicate(conn, new_account.id)) {
      printf("이미 있는 아이디입니다.\n");
      continue;
    }

    valid_id = 1;
  }

  // ── 비밀번호 입력 ──
  // 규칙: 영문+숫자 조합 필수, 허용 특수문자 !?$ 만 가능
  int valid_pw = 0;
  while (!valid_pw) {
    printf("비밀번호 (영문+숫자 필수, 특수문자는 !?$만 허용): ");
    scanf("%s", new_account.pw);

    int has_alpha = 0, has_digit = 0;
    int fmt_ok = validate_format(new_account.pw, &has_alpha, &has_digit);

    if (!fmt_ok) {
      printf("비밀번호 형식이 틀렸습니다. (허용 문자: 영문, 숫자, !?$)\n");
      continue;
    }
    if (!has_alpha || !has_digit) {
      printf("비밀번호 형식이 틀렸습니다. (영문자와 숫자를 반드시 포함해야 합니다)\n");
      continue;
    }
    valid_pw = 1;
  }

  // ── 닉네임 입력 ──
  // 규칙: 중복 불가
  int valid_nick = 0;
  while (!valid_nick) {
    printf("닉네임: ");
    scanf("%s", new_account.nickname);

    if (check_nickname_duplicate(conn, new_account.nickname)) {
      printf("이미 있는 닉네임입니다.\n");
      continue;
    }
    valid_nick = 1;
  }

  // ── 학번 입력 ──
  // 규칙: 1000000000 ~ 9999999999 (10자리), 중복 불가
  int valid_sid = 0;
  while (!valid_sid) {
    printf("학번 (10자리 숫자): ");
    while (getchar() != '\n'); // 이전 입력 개행 문자 제거

    if (scanf("%lld", &new_account.student_id) != 1) {
      int c;
      while ((c = getchar()) != '\n' && c != EOF);
      printf("숫자만 입력해주세요.\n");
      continue;
    }

    if (new_account.student_id < 1000000000LL ||
        new_account.student_id > 9999999999LL) {
      printf("학번은 정확히 10자리여야 합니다.\n");
      continue;
    }

    if (check_student_id_duplicate(conn, new_account.student_id)) {
      printf("이미 가입된 학번입니다.\n");
      continue;
    }
    valid_sid = 1;
  }

  // ── 기타 개인정보 입력 ──
  printf("이름: ");
  scanf("%s", new_account.name);

  printf("전공: ");
  scanf("%s", new_account.major);

  printf("전화번호 (예: 010-1234-5678): ");
  scanf("%s", new_account.phone);

  if (register_user(conn, new_account.id, new_account.pw,
                    new_account.nickname, new_account.student_id,
                    new_account.name, new_account.major,
                    new_account.phone)) {
    printf("회원가입이 성공적으로 완료되었습니다!\n");
  }
}
