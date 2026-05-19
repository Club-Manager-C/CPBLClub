#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include "clube_manage.h"

void club_manage_menu(MYSQL *conn) {

    int menu;

    while (1) {

        printf("\n===== 동아리 관리 =====\n");

        printf("1. 승인 대기 목록\n");
        printf("2. 동아리 승인\n");
        printf("3. 동아리 거절\n");
        printf("4. 동아리 삭제\n");
        printf("0. 종료\n");

        scanf("%d", &menu);

        switch(menu) {

            case 1:
                view_pending_clubs(conn);
                break;

            case 2:
                approve_club(conn);
                break;

            case 3:
                reject_club(conn);
                break;

            case 4:
                delete_club(conn);
                break;

            case 0:
                return;

            default:
                printf("잘못된 입력입니다.\n");
        }
    }
}


// ======================================================
// 승인 대기 동아리 조회
// ======================================================
void view_pending_clubs(MYSQL *conn) {

    MYSQL_RES *res;
    MYSQL_ROW row;

    char query[256] =
        "SELECT club_id, club_name, status "
        "FROM clubs "
        "WHERE status = '대기'";

    if (mysql_query(conn, query)) {

        printf("조회 실패 : %s\n", mysql_error(conn));
        return;
    }

    res = mysql_store_result(conn);

    printf("\n===== 승인 대기 동아리 =====\n");

    while ((row = mysql_fetch_row(res))) {

        printf("동아리 ID : %s\n", row[0]);
        printf("동아리명 : %s\n", row[1]);
        printf("상태 : %s\n", row[2]);

        printf("----------------------\n");
    }

    mysql_free_result(res);
}



// ======================================================
// 동아리 승인
// ======================================================
void approve_club(MYSQL *conn) {

    int club_id;

    printf("승인할 동아리 ID 입력 : ");
    scanf("%d", &club_id);

    char query[256];

    sprintf(query,
        "UPDATE clubs "
        "SET status = '승인' "
        "WHERE club_id = %d",
        club_id);

    if (mysql_query(conn, query)) {

        printf("승인 실패 : %s\n", mysql_error(conn));
        return;
    }

    printf("동아리 승인 완료\n");
}


// ======================================================
// 동아리 거절
// ======================================================
void reject_club(MYSQL *conn) {

    int club_id;
    char reason[255];

    printf("거절할 동아리 ID 입력 : ");
    scanf("%d", &club_id);

    getchar();

    printf("거절 사유 입력 : ");
    fgets(reason, sizeof(reason), stdin);

    reason[strcspn(reason, "\n")] = 0;

    if (strlen(reason) == 0) {

        printf("거절 사유는 필수 입력입니다.\n");
        return;
    }

    char query[512];

    sprintf(query,
        "UPDATE clubs "
        "SET status = '거절', "
        "reject_reason = '%s' "
        "WHERE club_id = %d",
        reason,
        club_id);

    if (mysql_query(conn, query)) {

        printf("거절 실패 : %s\n", mysql_error(conn));
        return;
    }

    printf("동아리 거절 완료\n");
}



// ======================================================
// 동아리 삭제
// ======================================================
void delete_club(MYSQL *conn) {

    int club_id;
    char check;

    printf("삭제할 동아리 ID 입력 : ");
    scanf("%d", &club_id);

    printf("정말 삭제하시겠습니까? (Y/N) : ");
    scanf(" %c", &check);

    if (check != 'Y' && check != 'y') {

        printf("삭제 취소\n");
        return;
    }

    char query[256];

    sprintf(query,
        "DELETE FROM clubs "
        "WHERE club_id = %d",
        club_id);

    if (mysql_query(conn, query)) {

        printf("삭제 실패 : %s\n", mysql_error(conn));
        return;
    }

    printf("동아리 삭제 완료\n");
}