#include "category.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//===========================================
//카테고리 메뉴
void category_menu(MYSQL *conn) {

    int menu;

    
    while (1) {

        printf("\n===== 카테고리 관리 =====\n");

        printf("1. 카테고리 조회\n");
        printf("2. 카테고리 추가\n");
        printf("3. 카테고리 수정\n");
        printf("4. 카테고리 삭제\n");
        printf("0. 종료\n");

        printf("=============================\n");
        printf("입력: ");

        scanf("%d", &menu);

        switch (menu) {

            case 1:
                view_categories(conn);
                break;

            case 2: 
                add_category(conn);
                break;

            case 3: 
                update_category(conn);
                break;

            case 4: 
                delete_category(conn);
                break;
            
            case 0: return;
            default: printf("잘못된 입력입니다.\n");
        }
    }
}


//==================================================
//카테고리 조회(ADM-CAT-01)
void view_categories(MYSQL *conn) {

    const char *query =
        "SELECT category_id, category_name "
        "FROM club_categories "
        "ORDER BY category_id ASC";
        //category_name -> category_id : 이름순 -> ID순

    if (mysql_query(conn, query)) {
        printf("카테고리 조회 실패: %s\n",
               mysql_error(conn));
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);

    MYSQL_ROW row;

    printf("\n===== 카테고리 목록 =====\n");

    while ((row = mysql_fetch_row(res))) {

        printf("[%s] %s\n",
               row[0],
               row[1]);
    }

    mysql_free_result(res);
}


//=====================================================
//카테고리 추가(ADM-CAT-02)
int add_category(MYSQL *conn) {

    char name[50];

    printf("추가할 이름: ");
    scanf("%s", name);

    char query[256];

    snprintf(query, sizeof(query),
        "SELECT * FROM club_categories "
        "WHERE category_name='%s'",
        name);

    mysql_query(conn, query);

    MYSQL_RES *res = mysql_store_result(conn);

    if (mysql_num_rows(res) > 0) {

        printf("이미 존재하는 카테고리입니다.\n");

        mysql_free_result(res);

        return 0;
    }

    mysql_free_result(res);

    snprintf(query, sizeof(query),
        "INSERT INTO club_categories(category_name) "
        "VALUES('%s')",
        name);

    if (mysql_query(conn, query)) {

        printf("카테고리 추가 실패: %s\n",
               mysql_error(conn));

        return 0;
    }

    printf("카테고리 추가 완료\n");

    return 1;
}


//=======================================================
//카테고리 변경(ADM-CAT-03)
int update_category(MYSQL *conn) {

    int id;
    char new_name[50];
    char query[256];
    
    printf("수정할 ID: ");
    scanf("%d", &id);
    printf("새 이름: ");
    scanf("%s", new_name);

    snprintf(query, sizeof(query),
        "UPDATE club_categories "
        "SET category_name='%s' "
        "WHERE category_id=%d",
        new_name,
        id);

    if (mysql_query(conn, query)) {

        printf("카테고리 수정 실패: %s\n",
               mysql_error(conn));

        return 0;
    }

    printf("카테고리 수정 완료\n");

    return 1;
}


//========================================================
//카테고리 삭제(ADM-CAT-04)
int delete_category(MYSQL *conn) {

    int id;
    printf("삭제할 ID: ");
    scanf("%d", &id);

    char query[256];

    snprintf(query, sizeof(query),
        "SELECT * FROM clubs "
        "WHERE category_id=%d",
        id);

    mysql_query(conn, query);

    MYSQL_RES *res = mysql_store_result(conn);

    if (mysql_num_rows(res) > 0) {

        printf("해당 카테고리를 사용하는 동아리가 존재합니다.\n");

        mysql_free_result(res);

        return 0;
    }

    mysql_free_result(res);

    printf("정말 삭제하시겠습니까? (y/n): ");

    char ch;
    scanf(" %c", &ch);

    if (ch != 'y' && ch != 'Y') {

        printf("삭제 취소\n");

        return 0;
    }

    snprintf(query, sizeof(query),
        "DELETE FROM club_categories "
        "WHERE category_id=%d",
        id);

    if (mysql_query(conn, query)) {

        printf("삭제 실패: %s\n",
               mysql_error(conn));

        return 0;
    }

    printf("삭제 완료\n");

    return 1;
}