#ifndef FILTER_H
#define FILTER_H

#include <stdio.h>

/**
 * @brief 텍스트 파일로부터 욕설 목록을 읽어와 메모리에 로드합니다.
 * @param filename 욕설 목록이 저장된 파일 경로 (기본값 "slang_list.txt")
 * @return 로드된 욕설 개수, 파일 읽기 실패 시 -1 반환
 */
int load_slang_list(const char *filename);

/**
 * @brief 입력받은 문자열에 로드된 욕설 키워드가 포함되어 있는지 검사합니다.
 * @param text 검사할 문자열 (게시글 제목, 내용, 댓글 등)
 * @return 욕설 포함 시 1(참), 미포함 시 0(거짓) 반환
 */
int contains_slang(const char *text);

/**
 * @brief 메모리에 로드된 욕설 목록을 해제하고 자원을 반환합니다.
 */
void free_slang_list(void);

#endif /* FILTER_H */
