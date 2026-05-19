#include "db.h"
#include "category.h"
#include "clube_manage.h"

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
                club_manage_menu(conn);
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


void run_admin_interface(MYSQL *conn) {
    // 카테고리는 db.c의 init_db에서 초기화되므로 생략합니다.

    //동아리 승인 대기 목록 생성 (테스트용)
// 예시: category_id 1(예: 스포츠, 학술 등)이 이미 생성되어 있다고 가정
    int res = mysql_query(conn,
            "INSERT IGNORE INTO clubs "
            "(club_name, category_id, leader_id, purpose, status, apply_date) "
            "VALUES "
            "('inherit', 1, 'admin', '테스트 목적', '대기', '2026-05-19 12:00:00'),"
            "('basketball', 1, 'admin', '농구 동아리', '대기', '2026-05-19 12:00:00'),"
            "('soccer', 1, 'admin', '축구 동아리', '대기', '2026-05-19 12:00:00')"
        );

    // C언어에서 MySQL을 다룰 때 꿀팁: 에러 확인 코드 추가
    if (res != 0) {
        printf("더미 데이터 삽입 실패: %s\n", mysql_error(conn));
    } else {
    printf("더미 데이터 삽입 성공!\n");
}
    printf("총관리자 로그인 성공!\n");
    admin_home_screen(conn);
}



