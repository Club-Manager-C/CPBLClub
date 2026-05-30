#ifndef MAJOR_INFO_H
#define MAJOR_INFO_H

// 단과대 이름을 반환하는 함수
const char* get_college_name(int college_code);

// 세부 전공 이름을 반환하는 함수
const char* get_major_name(int college_code, int major_code);

// 사용자에게 단과대 및 전공 선택 메뉴를 보여주고 코드를 입력받는 함수
void select_college_and_major(int *out_college, int *out_major);

#endif // MAJOR_INFO_H
