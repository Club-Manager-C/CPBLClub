#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 관리자 메인 메뉴
void admin_home_screen(MYSQL *conn) {
    int choice;

    while (1) {
        printf("\n============================\n");
        printf("  총관리자 메뉴\n");
        printf("============================\n");
        printf("1. 동아리 등록 기간 설정\n");
        printf("2. 동아리 관리\n");
        printf("3. 동아리장 승인\n");
        printf("4. 동아리 카테고리 설정\n");
        printf("0. 종료\n");
        printf("============================\n");
        printf("입력: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("\n[동아리 등록 기간 설정]\n(준비 중입니다)\n");
                break;
            case 2:
                printf("\n[동아리 관리]\n(준비 중입니다)\n");
                break;
            case 3:
                printf("\n[동아리장 승인]\n(준비 중입니다)\n");
                break;
            case 4:
                printf("\n[동아리 카테고리 설정]\n(준비 중입니다)\n");
                break;
            case 0:
                printf("관리자 프로그램을 종료합니다.\n");
                return;
            default:
                printf("잘못된 입력입니다.\n");
        }
    }
}



