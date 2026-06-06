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