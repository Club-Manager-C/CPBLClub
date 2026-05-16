#include "db.h"
#include "category.h"

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
                category_menu(conn);
                break;
            case 0:
                printf("관리자 프로그램을 종료합니다.\n");
                return;
            default:
                printf("잘못된 입력입니다.\n");
        }
    }
}

int main() {
    MYSQL *conn;
    init_db(&conn);

    char id[50];
    char pw[50];
    
    // 임의의 총관리자 계정
    const char *ADMIN_ID = "admin";
    const char *ADMIN_PW = "admin1234";

    //카테고리 목록 초기화
    mysql_query(conn,
        "INSERT INTO club_categories(category_name) "
        "VALUES ('전공'), ('밴드'), ('댄스'), ('봉사'), ('취미'), ('운동')");


    printf("\n=== 총관리자 프로그램 ===\n");
    printf("아이디: ");
    scanf("%s", id);
    printf("비밀번호: ");
    scanf("%s", pw);

    if (strcmp(id, ADMIN_ID) == 0 && strcmp(pw, ADMIN_PW) == 0) {
        printf("총관리자 로그인 성공!\n");
        admin_home_screen(conn);
    } else {
        printf("아이디 또는 비밀번호가 틀렸습니다.\n");
    }

    
    close_db(conn);
    return 0;
}

