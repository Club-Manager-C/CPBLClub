#include "db.h"
#include "category.h"
#include "clube_manage.h"
#include "apply_period.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void manageClubRequests(MYSQL *conn) {
    char query[1024];
    
    while (1) {
        printf("\n================================================\n");
        printf("  동아리 개설 및 동아리장 승인 대기 목록\n");
        printf("==================================================\n");
        
        // clubs 테이블과 users 테이블을 leader_id로 JOIN하여 대기 중인 목록 조회
        sprintf(query, 
            "SELECT c.club_id, c.club_name, c.leader_id, "
            "u.name, u.student_id, c.apply_date "
            "FROM clubs c "
            "JOIN users u ON c.leader_id = u.id "
            "WHERE c.status = '대기' "
            "ORDER BY c.apply_date ASC"
        );
        
        if (mysql_query(conn, query)) {
            fprintf(stderr, "대기 목록 조회 실패: %s\n", mysql_error(conn));
            return;
        }
        
        MYSQL_RES *res = mysql_store_result(conn);
        if (res == NULL) {
            fprintf(stderr, "결과셋 로드 실패: %s\n", mysql_error(conn));
            return;
        }
        
        int row_count = mysql_num_rows(res);
        if (row_count == 0) {
            printf("현재 대기 중인 동아리 신청 요청이 없습니다.\n");
            mysql_free_result(res);
            return;
        }
        
        printf("%-8s %-20s %-15s %-15s %-20s\n", "신청ID", "동아리명", "신청자명", "학번", "신청일시");
        printf("--------------------------------------------------------------------------------\n");
        
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            printf("%-8s %-20s %-15s %-15s %-20s\n", 
                   row[0], row[1], row[3], row[4], row[5]);
        }
        
        mysql_free_result(res);
        
        int target_club_id;
        printf("\n승인/거절 처리할 신청 ID (club_id) 입력 (0. 뒤로가기): ");
        if (scanf("%d", &target_club_id) != 1) {
            while (getchar() != '\n');
            printf("잘못된 숫자 입력입니다.\n");
            continue;
        }
        if (target_club_id == 0) return;
        
        // 입력한 club_id가 실제로 존재하는지 대기 상태인지 검증
        sprintf(query, "SELECT club_name, leader_id FROM clubs WHERE club_id = %d AND status = '대기'", target_club_id);
        if (mysql_query(conn, query)) {
            fprintf(stderr, "검증 쿼리 실패: %s\n", mysql_error(conn));
            continue;
        }
        MYSQL_RES *check_res = mysql_store_result(conn);
        if (check_res == NULL || mysql_num_rows(check_res) == 0) {
            printf("대기 중인 해당 신청 ID가 존재하지 않습니다.\n");
            if (check_res) mysql_free_result(check_res);
            continue;
        }
        MYSQL_ROW check_row = mysql_fetch_row(check_res);
        char club_name[100];
        char leader_id[50];
        strcpy(club_name, check_row[0]);
        strcpy(leader_id, check_row[1]);
        mysql_free_result(check_res);
        
        int approval_choice;
        printf("\n선택하신 동아리: %s (신청자 ID: %s)\n", club_name, leader_id);
        printf("1. 승인  2. 거절  0. 취소\n입력: ");
        if (scanf("%d", &approval_choice) != 1) {
            while (getchar() != '\n');
            printf("잘못된 입력입니다.\n");
            continue;
        }
        
        if (approval_choice == 1) {
            // [승인 시 처리 로직]
            sprintf(query, "UPDATE clubs SET status = '승인' WHERE club_id = %d", target_club_id);
            if (mysql_query(conn, query)) {
                fprintf(stderr, "동아리 승인 업데이트 실패: %s\n", mysql_error(conn));
                continue;
            }
            
            // 기존 users 테이블에 is_club_leader 컬럼이 없으므로 권한 변경은 생략하거나 필요에 따라 대응
            
            sprintf(query, "INSERT IGNORE INTO clubmembers (club_id, user_idx, role) VALUES (%d, (SELECT user_idx FROM users WHERE id = '%s'), 'Leader')", target_club_id, leader_id);
            // 만약 clubmembers 테이블이 구버전이면 user_idx 대신 user_id를 씁니다. 만약 에러가 난다면 SELECT user_idx로 가져와서 삽입하도록 쿼리를 유연하게 작성
            if (mysql_query(conn, query)) {
                // 구버전 컬럼 대응을 위한 예비 쿼리 작동 (user_id 컬럼인 경우)
                sprintf(query, "INSERT IGNORE INTO clubmembers (club_id, user_id, role) VALUES (%d, '%s', 'Leader')", target_club_id, leader_id);
                mysql_query(conn, query);
            }
            
            char msg_content[500];
            sprintf(msg_content, "축하합니다! [%s] 등록 신청이 승인되었습니다.", club_name);
            insert_message(conn, leader_id, msg_content);
            
            printf("✅ [%s] 동아리 신청이 최종 '승인'되었습니다!\n", club_name);
            
        } else if (approval_choice == 2) {
            // [거절 시 처리 로직]
            char reject_reason[256];
            printf("거절 사유 입력 (최대 250자): ");
            while (getchar() != '\n'); // 버퍼 비우기
            fgets(reject_reason, sizeof(reject_reason), stdin);
            reject_reason[strcspn(reject_reason, "\n")] = '\0';
            
            char escaped_reason[512];
            mysql_real_escape_string(conn, escaped_reason, reject_reason, strlen(reject_reason));
            
            sprintf(query, "UPDATE clubs SET status = '거절', reject_reason = '%s' WHERE club_id = %d", escaped_reason, target_club_id);
            if (mysql_query(conn, query)) {
                fprintf(stderr, "동아리 거절 업데이트 실패: %s\n", mysql_error(conn));
                continue;
            }
            
            char msg_content[500];
            sprintf(msg_content, "[%s] 등록 신청이 거절되었습니다. 사유: %s", club_name, reject_reason);
            insert_message(conn, leader_id, msg_content);
            
            printf("❌ [%s] 동아리 신청이 '거절'되었습니다.\n", club_name);
        } else {
            printf("취소되었습니다.\n");
        }
    }
}

// 관리자 메인 메뉴
void admin_home_screen(MYSQL *conn) {
    int choice;

    while (1) {
        printf("\n============================\n");
        printf("  총관리자 메뉴\n");
        printf("============================\n");
        printf("1. 동아리 등록 기간 설정\n");
        printf("2. 동아리 삭제\n");
        printf("3. 동아리 개설 신청 관리\n");
        printf("4. 동아리 카테고리 설정\n");
        printf("0. 종료\n");
        printf("============================\n");
        printf("입력: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                manage_apply_period(conn);
                break;
            case 2:
                delete_club(conn);
                break;
            case 3:
                manageClubRequests(conn);
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
    printf("총관리자 로그인 성공!\n");
    admin_home_screen(conn);
}



