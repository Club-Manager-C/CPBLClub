#include <stdio.h>
#include <stdlib.h>
#include "decorate.h"

void print_main_menu() {
    system("cls"); // 화면 지우기, 필요 없으면 삭제 가능

    printf("╔════════════════════════════════════════════╗\n");
    printf("║                                            ║\n");
    printf("║              CPBL 동아리 시스템            ║\n");
    printf("║                                            ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║                                            ║\n");
    printf("║    1. 로그인                               ║\n");
    printf("║                                            ║\n");
    printf("║    2. 회원가입                             ║\n");
    printf("║                                            ║\n");
    printf("║    0. 종료                                 ║\n");
    printf("║                                            ║\n");
    printf("╚════════════════════════════════════════════╝\n");
    printf("\n선택 > ");
}

void print_user_menu() {
    system("cls");

    printf("╔════════════════════════════════════════════╗\n");
    printf("║                                            ║\n");
    printf("║                 메 인  메 뉴               ║\n");
    printf("║                                            ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║                                            ║\n");
    printf("║    1. 동아리 목록  (카테고리별)            ║\n");
    printf("║                                            ║\n");
    printf("║    2. 마이페이지                           ║\n");
    printf("║                                            ║\n");
    printf("║    3. 동아리 개설 신청                     ║\n");
    printf("║                                            ║\n");
    printf("║    0. 로그아웃                             ║\n");
    printf("║                                            ║\n");
    printf("╚════════════════════════════════════════════╝\n");
    printf("\n선택 > ");
}

void wait_enter_and_clear(const char *message) {
    int c;

    printf("\n%s", message);

    // scanf 후 입력 버퍼에 남은 엔터 제거
    while ((c = getchar()) != '\n' && c != EOF);

    // 사용자가 Enter 누를 때까지 대기
    getchar();

    // 화면 지우기
    system("cls");
}

int utf8_char_width(const char *s) {
    unsigned char c = (unsigned char)s[0];

    if (c < 0x80) {
        return 1; // 영어, 숫자
    }

    return 2; // 한글 등은 화면에서 2칸으로 처리
}

int utf8_char_len(const char *s) {
    unsigned char c = (unsigned char)s[0];

    if (c < 0x80) return 1;
    else if ((c & 0xE0) == 0xC0) return 2;
    else if ((c & 0xF0) == 0xE0) return 3;
    else if ((c & 0xF8) == 0xF0) return 4;

    return 1;
}

void print_fixed_utf8(const char *s, int width) {
    int used = 0;

    while (*s) {
        int len = utf8_char_len(s);
        int w = utf8_char_width(s);

        if (used + w > width) {
            break;
        }

        for (int i = 0; i < len; i++) {
            putchar(s[i]);
        }

        s += len;
        used += w;
    }

    while (used < width) {
        putchar(' ');
        used++;
    }
}