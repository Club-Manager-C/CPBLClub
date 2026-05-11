#include "db.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char id[50];
  char pw[50];
  char nickname[50];
  char student_id[12]; // 10자리 학번
  char name[50];       // 이름
  char major[50];      // 전공
  char phone[20];      // 전화번호
} Account;

// 영문+숫자+허용 특수문자(!?$)만 포함하는지 검사
// has_alpha: 영문자 포함 여부, has_digit: 숫자 포함 여부 반환 (포인터)
// 반환값: 1 = 형식 OK, 0 = 허용되지 않는 문자 포함
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

void home_screen() {
  printf("\n=== 홈 화면 ===\n");
  printf("홈 화면에 입장했습니다.\n");
  // TODO: 홈 화면 기능 구현
}

int main() {
  MYSQL *conn;
  init_db(&conn);


  int choice;
  int is_logged_in = 0;

  while (1) {
    printf("\n=== 시작 화면 ===\n");
    printf("1. 로그인\n");
    printf("2. 회원가입\n");
    printf("3. 종료\n");
    printf("입력: ");
    scanf("%d", &choice);

    if (choice == 1) {
      int fail_count = 0;
      char id[50];
      char pw[50];

      while (fail_count < 5) {
        printf("\n아이디: ");
        scanf("%s", id);
        printf("PW: ");
        scanf("%s", pw);

        if (check_login(conn, id, pw)) {
          printf("성공했습니다.\n");
          is_logged_in = 1;
          break;
        } else {
          fail_count++;
          printf("아이디 혹은 PW를 잘 못 입력했습니다.\n");
          printf("틀린 횟수: %d\n", fail_count);
        }
      }

      if (fail_count == 5) {
        printf("5번 틀렸습니다.\n");
        continue;
      }

      if (is_logged_in) {
        home_screen();
        break;
      }
    } else if (choice == 2) {
      Account new_account;
      int valid_id = 0;

      // ── 아이디 입력 ──
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

      // ── 학번 입력 (RBQ-SU-04) ──
      // 규칙: 정확히 10자리 숫자, 중복 불가
      int valid_sid = 0;
      while (!valid_sid) {
        printf("학번 (10자리): ");
        scanf("%s", new_account.student_id);

        int len = (int)strlen(new_account.student_id);
        if (len != 10) {
          printf("학번은 정확히 10자리여야 합니다. (현재 %d자리)\n", len);
          continue;
        }

        int all_digit = 1;
        for (int i = 0; i < len; i++) {
          if (!isdigit((unsigned char)new_account.student_id[i])) {
            all_digit = 0;
            break;
          }
        }
        if (!all_digit) {
          printf("학번은 숫자만 입력해주세요.\n");
          continue;
        }

        if (check_student_id_duplicate(conn, new_account.student_id)) {
          printf("이미 가입된 학번입니다.\n");
          continue;
        }
        valid_sid = 1;
      }

      // ── 기타 개인정보 입력 (RBQ-SU-05) ──
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
    } else if (choice == 3) {
      printf("프로그램을 종료합니다.\n");
      break;
    } else {
      printf("잘못된 입력입니다.\n");
    }
  }

  close_db(conn);
  return 0;
}