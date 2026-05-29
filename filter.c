#include "filter.h"
#include <stdlib.h>
#include <string.h>

static char **slang_list = NULL;
static int slang_count = 0;
static int slang_capacity = 0;

int load_slang_list(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("slang_list.txt 파일을 열 수 없습니다");
        return -1;
    }

    char buffer[256];
    
    // 기존에 로드된 리스트가 있다면 해제
    free_slang_list();

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        // 개행 문자(\n, \r) 제거
        buffer[strcspn(buffer, "\r\n")] = '\0';

        // 공백 라인은 제외
        if (strlen(buffer) == 0) {
            continue;
        }

        // 동적 배열 공간 늘리기
        if (slang_count >= slang_capacity) {
            slang_capacity = (slang_capacity == 0) ? 16 : slang_capacity * 2;
            char **temp = realloc(slang_list, slang_capacity * sizeof(char *));
            if (temp == NULL) {
                perror("메모리 재할당 실패");
                fclose(file);
                free_slang_list();
                return -1;
            }
            slang_list = temp;
        }

        // 키워드 복사 및 저장
        slang_list[slang_count] = strdup(buffer);
        if (slang_list[slang_count] == NULL) {
            perror("메모리 할당 실패 (strdup)");
            fclose(file);
            free_slang_list();
            return -1;
        }
        slang_count++;
    }

    fclose(file);
    return slang_count;
}

int contains_slang(const char *text) {
    if (text == NULL || slang_list == NULL || slang_count == 0) {
        return 0;
    }

    for (int i = 0; i < slang_count; i++) {
        // 문자열에 욕설 키워드가 부분 검색으로 포함되어 있는지 확인
        if (strstr(text, slang_list[i]) != NULL) {
            return 1; // 욕설 발견
        }
    }

    return 0; // 욕설 없음
}

void free_slang_list(void) {
    if (slang_list != NULL) {
        for (int i = 0; i < slang_count; i++) {
            free(slang_list[i]);
        }
        free(slang_list);
        slang_list = NULL;
    }
    slang_count = 0;
    slang_capacity = 0;
}
