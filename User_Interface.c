#include "db.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char id[50];
  char pw[50];
  char nickname[50];
  long long student_id; // 10자리 학번
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

// ─────────────────────────────────────────────
// 메뉴별 스텁 함수 (추후 각 파일에서 구현 예정)
// ─────────────────────────────────────────────

void promo_board() {
  printf("\n=== 동아리 홍보 게시판 ===\n");
  printf("(준비 중입니다)\n");
}

void major_club_board() {
  printf("\n=== 전공 동아리 게시판 ===\n");
  printf("(준비 중입니다)\n");
}

// ─────────────────────────────────────────────
// 마이페이지 헬퍼 함수① — 내 게시글 확인/수정
// ─────────────────────────────────────────────
static void my_posts_menu(MYSQL *conn, const char *logged_id) {
  int post_id;
  while (1) {
    printf("\n=== 내 게시글 확인/수정 ===\n");
    get_my_posts(conn, logged_id);
    printf("\n수정할 게시글 ID 입력 (0: 뒤로): ");
    scanf("%d", &post_id);
    if (post_id == 0) return;

    char new_content[500];
    printf("새 내용 입력: ");
    while (getchar() != '\n'); // 버퍼 정리
    fgets(new_content, sizeof(new_content), stdin);
    new_content[strcspn(new_content, "\n")] = '\0';

    if (update_post(conn, post_id, logged_id, new_content)) {
      printf("✅ 수정 완료!\n");
    } else {
      printf("❌ 수정 실패 (본인 글이 아니거나 존재하지 않는 ID)\n");
    }
  }
}

// ─────────────────────────────────────────────
// 마이페이지 헬퍼 함수② — 내 댓글 확인/수정
// ─────────────────────────────────────────────
static void my_comments_menu(MYSQL *conn, const char *logged_id) {
  int comment_id;
  while (1) {
    printf("\n=== 내 댓글 확인/수정 ===\n");
    get_my_comments(conn, logged_id);
    printf("\n수정할 댓글 ID 입력 (0: 뒤로): ");
    scanf("%d", &comment_id);
    if (comment_id == 0) return;

    char new_content[500];
    printf("새 내용 입력: ");
    while (getchar() != '\n');
    fgets(new_content, sizeof(new_content), stdin);
    new_content[strcspn(new_content, "\n")] = '\0';

    if (update_comment(conn, comment_id, logged_id, new_content)) {
      printf("✅ 수정 완료!\n");
    } else {
      printf("❌ 수정 실패 (본인 댓글이 아니거나 존재하지 않는 ID)\n");
    }
  }
}

// ─────────────────────────────────────────────
// 마이페이지 헬퍼 함수③ — 시간표 확인/수정
// ─────────────────────────────────────────────
static void my_schedule_menu(MYSQL *conn, const char *logged_id) {
  int choice;
  while (1) {
    printf("\n=== 시간표 확인/수정 ===\n");
    get_my_schedule(conn, logged_id);
    printf("\n1. 추가  2. 삭제  0. 뒤로\n입력: ");
    scanf("%d", &choice);

    if (choice == 0) {
      return;
    } else if (choice == 1) {
      char day[10], start[10], end_t[10], subject[100], location[100];
      printf("요일 (월/화/수/목/금/토/일): ");
      scanf("%9s", day);
      printf("시작시간 (예: 09:00): ");
      scanf("%9s", start);
      printf("종료시간 (예: 10:30): ");
      scanf("%9s", end_t);
      printf("과목명: ");
      scanf("%99s", subject);
      printf("강의실: ");
      scanf("%99s", location);
      if (add_schedule(conn, logged_id, day, start, end_t, subject, location)) {
        printf("✅ 추가 완료!\n");
      } else {
        printf("❌ 추가 실패\n");
      }
    } else if (choice == 2) {
      int sid;
      printf("삭제할 시간표 ID 입력: ");
      scanf("%d", &sid);
      if (delete_schedule(conn, sid, logged_id)) {
        printf("✅ 삭제 완료!\n");
      } else {
        printf("❌ 삭제 실패 (본인 항목이 아니거나 존재하지 않는 ID)\n");
      }
    } else {
      printf("잘못된 입력\n");
    }
  }
}

// ─────────────────────────────────────────────
// 마이페이지 메인
// ─────────────────────────────────────────────
void my_page(MYSQL *conn, const char *logged_id) {
  int choice;
  while (1) {
    printf("\n============================\n");
    printf("  마이페이지 (%s)\n", logged_id);
    printf("============================\n");
    printf("1. 내 게시글 확인/수정\n");
    printf("2. 내 댓글 확인/수정\n");
    printf("3. 시간표 확인/수정\n");
    printf("0. 뒤로\n");
    printf("============================\n");
    printf("입력: ");
    scanf("%d", &choice);

    switch (choice) {
      case 1: my_posts_menu(conn, logged_id);    break;
      case 2: my_comments_menu(conn, logged_id); break;
      case 3: my_schedule_menu(conn, logged_id); break;
      case 0: return;
      default: printf("잘못된 입력입니다.\n");
    }
  }
}

// ─────────────────────────────────────────────
// 동아리장 신청 (스텁)
// ─────────────────────────────────────────────
void apply_club_leader() {
  printf("\n=== 동아리장 신청 ===\n");
  printf("(준비 중입니다)\n");
}

// ─────────────────────────────────────────────
// 메인 화면 (로그인 후 진입)
// ─────────────────────────────────────────────
void home_screen(MYSQL *conn, const char *logged_id) {
  int choice;

  while (1) {
    printf("\n============================\n");
    printf("  메인 메뉴\n");
    printf("============================\n");
    printf("1. 동아리 홍보 게시판\n");
    printf("2. 전공 동아리 게시판\n");
    printf("3. 마이페이지\n");
    printf("4. 동아리장 신청\n");
    printf("0. 로그아웃\n");
    printf("============================\n");
    printf("입력: ");
    scanf("%d", &choice);

    switch (choice) {
      case 1:
        promo_board();
        break;
      case 2:
        major_club_board();
        break;
      case 3:
        my_page(conn, logged_id);
        break;
      case 4:
        apply_club_leader();
        break;
      case 0:
        printf("로그아웃 합니다.\n");
        return;
      default:
        printf("잘못된 입력입니다.\n");
    }
  }
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
        home_screen(conn, id);
        is_logged_in = 0; // 로그아웃 후 시작 화면으로 복귀
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
      // 규칙: 1000000000 ~ 9999999999 (10자리), 중복 불가
      int valid_sid = 0;
      while (!valid_sid) {
        printf("학번 (10자리 숫자): ");
        // 이전 입력에서 남은 개행 문자(\n) 제거
        while (getchar() != '\n'); 

        if (scanf("%lld", &new_account.student_id) != 1) {
          // 숫자가 아닌 입력 처리
          int c;
          while ((c = getchar()) != '\n' && c != EOF); // 버퍼 비우기
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