#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include "clube_manage.h"

// ======================================================
// 동아리 삭제
// ======================================================
void delete_club(MYSQL *conn) {

    MYSQL_RES *res;
    MYSQL_ROW row;

    // 현재 등록된 동아리 목록 조회
    char list_query[256] =
        "SELECT club_id, club_name, status "
        "FROM clubs";

    if (mysql_query(conn, list_query)) {

        printf("동아리 조회 실패 : %s\n", mysql_error(conn));
        return;
    }

    res = mysql_store_result(conn);

    printf("\n===== 현재 등록된 동아리 목록 =====\n");

    while ((row = mysql_fetch_row(res))) {

        printf("동아리 ID : %s\n", row[0]);
        printf("동아리명 : %s\n", row[1]);
        printf("상태 : %s\n", row[2]);

        printf("----------------------\n");
    }

    mysql_free_result(res);

    // 삭제 진행
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