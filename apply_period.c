#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>

#include "apply_period.h"
#include "db.h"


//=======================================================
// 모집 기간 관리 메인 메뉴
//=======================================================
void manage_apply_period(MYSQL *conn) {

    int choice;

    while (1) {

        printf("\n");
        printf("====================================\n");
        printf("        모집 기간 관리 메뉴\n");
        printf("====================================\n");

        printf("1. 현재 모집 기간 조회\n");
        printf("2. 모집 기간 설정(등록 + 수정)\n");
        printf("3. 모집 기간 삭제\n");
        printf("0. 돌아가기\n");

        printf("====================================\n");

        printf("입력 : ");
        scanf("%d", &choice);

        switch (choice) {

            case 1:
                show_apply_period(conn);
                break;

            case 2:
                save_apply_period(conn);
                break;

            case 3: 
                delete_apply_period(conn);
                break;

            case 0:
                return;

            default:
                printf("잘못된 입력입니다. 다시 시도해주세요.\n");
        }
    }
}


//=======================================================
// 현재 모집 기간 출력
//=======================================================
void show_apply_period(MYSQL *conn) {

    char query[300];

    sprintf(query,
        "SELECT period_id, label, start_date, end_date "
        "FROM recruitment_periods "
        "WHERE is_active = 1");

    if (mysql_query(conn, query)) {

        printf("기간 조회 실패 : %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);

    if (res == NULL) {

        printf("조회 결과 없음\n");
        return;
    }

    MYSQL_ROW row = mysql_fetch_row(res);

    if (row != NULL) {

        printf("\n===== 현재 모집 기간 =====\n");

        printf("기간 ID : %s\n", row[0]);
        printf("모집명   : %s\n", row[1]);
        printf("시작일   : %s\n", row[2]);
        printf("종료일   : %s\n", row[3]);
    }
    else {

        printf("\n현재 등록된 모집 기간이 없습니다.\n");
    }

    mysql_free_result(res);
}


//============================================
// 모집 기간 설정 (등록 + 수정 통합)
//============================================
void save_apply_period(MYSQL *conn) {

    char label[50];
    char start_date[20];
    char end_date[20];

    printf("\n===== 모집 기간 설정 =====\n");

    printf("기간 이름 입력: ");

    getchar();

    fgets(label, sizeof(label), stdin);

    label[strcspn(label, "\n")] = 0;

    printf("시작 날짜 입력 (YYYY-MM-DD): ");
    scanf("%s", start_date);

    printf("종료 날짜 입력 (YYYY-MM-DD): ");
    scanf("%s", end_date);

    // 날짜 검증
    if (strcmp(start_date, end_date) > 0) {

        printf("\n종료 날짜는 시작 날짜보다 늦어야 합니다.\n");
        return;
    }

    // 기존 기간 존재 여부 확인
    char check_query[300];

    sprintf(check_query,
        "SELECT period_id FROM recruitment_periods LIMIT 1");

    if (mysql_query(conn, check_query)) {

        printf("조회 실패: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);

    MYSQL_ROW row = mysql_fetch_row(res);

    char query[500];

    //=========================
    // 기존 기간 존재 -> UPDATE
    //=========================
    if (row != NULL) {

        sprintf(query,
            "UPDATE recruitment_periods "
            "SET label='%s', "
            "start_date='%s', "
            "end_date='%s', "
            "is_active=1 "
            "WHERE period_id=%s",
            label,
            start_date,
            end_date,
            row[0]);

        if (mysql_query(conn, query)) {

            printf("\n기간 수정 실패: %s\n",
                mysql_error(conn));
        }
        else {

            printf("\n모집 기간 수정 완료!\n");
        }
    }

    //========================
    // 기존 기간 없음 -> INSERT
    //========================
    else {

        sprintf(query,
            "INSERT INTO recruitment_periods "
            "(label, start_date, end_date, is_active) "
            "VALUES ('%s', '%s', '%s', 1)",
            label,
            start_date,
            end_date);

        if (mysql_query(conn, query)) {

            printf("\n기간 등록 실패: %s\n",
                mysql_error(conn));
        }
        else {

            printf("\n모집 기간 등록 완료!\n");
        }
    }

    mysql_free_result(res);
}


//============================================
// 모집 기간 삭제
//============================================
void delete_apply_period(MYSQL *conn) {

    char query[300];

    // 기존 모집 기간 존재 여부 확인
    sprintf(query,
        "SELECT period_id, label "
        "FROM recruitment_periods "
        "LIMIT 1");

    if (mysql_query(conn, query)) {

        printf("\n조회 실패: %s\n",
            mysql_error(conn));

        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);

    MYSQL_ROW row = mysql_fetch_row(res);

    //=================
    // 모집 기간 없음
    //=================
    if (row == NULL) {

        printf("\n삭제할 모집 기간이 없습니다.\n");

        mysql_free_result(res);

        return;
    }

    //=================
    // 현재 기간 출력
    //=================
    printf("\n===== 현재 모집 기간 =====\n");

    printf("기간 ID : %s\n", row[0]);
    printf("기간명   : %s\n", row[1]);

    int choice;

    printf("\n정말 삭제하시겠습니까?\n");
    printf("1. 예\n");
    printf("0. 아니오\n");

    printf("입력 : ");
    scanf("%d", &choice);

    //==============
    // 삭제 진행
    //==============
    if (choice == 1) {

        char delete_query[300];

        sprintf(delete_query,
            "DELETE FROM recruitment_periods "
            "WHERE period_id=%s",
            row[0]);

        if (mysql_query(conn, delete_query)) {

            printf("\n삭제 실패: %s\n",
                mysql_error(conn));
        }
        else {

            printf("\n모집 기간 삭제 완료!\n");
        }
    }
    else {

        printf("\n삭제 취소\n");
    }

    mysql_free_result(res);
}



//=======================================================
// 신청 기간 열려있는지 확인
// 열려있으면 1 반환
// 아니면 0 반환
//=======================================================
int is_apply_period_open(MYSQL *conn) {

    char query[300];

    sprintf(query,
        "SELECT * "
        "FROM recruitment_periods "
        "WHERE CURDATE() BETWEEN start_date AND end_date "
        "AND is_active = 1");

    // SQL 실행 실패
    if (mysql_query(conn, query)) {

        printf("기간 조회 실패: %s\n", mysql_error(conn));
        return 0;
    }

    // 결과 저장
    MYSQL_RES *res = mysql_store_result(conn);

    if (res == NULL) {
        return 0;
    }

    // 결과 한 줄 가져오기
    MYSQL_ROW row = mysql_fetch_row(res);

    // 모집 기간 존재 여부 저장
    int result = (row != NULL);

    // 메모리 해제
    mysql_free_result(res);

    return result;
}