#include <stdio.h>
#include <string.h>

// DB가 없으므로 임시로 사용할 테스트 계정
const char* MOCK_ID = "test";
const char* MOCK_PW = "1234";

void home_screen() {
    printf("\n=== 홈 화면 ===\n");
    printf("홈 화면에 입장했습니다.\n");
    // TODO: 홈 화면 기능 구현
}

int main() {
    int choice;
    int is_logged_in = 0;

    while (1) {
        printf("\n=== 시작 화면 ===\n");
        printf("1. 로그인\n");
        printf("2. 종료\n");
        printf("입력: ");
        scanf("%d", &choice);

        if (choice == 1) {
            int fail_count = 0;
            char id[50];
            char pw[50];

            while (fail_count < 5) {
                printf("\n아이디: ");
                scanf("%s", id);
                printf("PW: ");
                scanf("%s", pw);

                // DB 연동 제외, 임시 계정과 비교
                if (strcmp(id, MOCK_ID) == 0 && strcmp(pw, MOCK_PW) == 0) {
                    printf("성공했습니다.\n");
                    is_logged_in = 1;
                    break;
                } else {
                    fail_count++;
                    printf("아이디 혹은 PW를 잘 못 입력했습니다.\n");
                    printf("틀린 횟수: %d\n", fail_count);
                }
            }

            if (fail_count == 5) {
                printf("5번 틀렸습니다.\n");
                // 시작화면으로 돌아감 (루프 계속)
                continue;
            }

            if (is_logged_in) {
                // 홈화면으로 이동
                home_screen();
                // 임시로 홈 화면 구경 후 프로그램 종료하도록 처리
                break; 
            }
        } else if (choice == 2) {
            printf("프로그램을 종료합니다.\n");
            break;
        } else {
            printf("잘못된 입력입니다.\n");
        }
    }

    return 0;
}