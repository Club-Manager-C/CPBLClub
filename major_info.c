#include "major_info.h"
#include <stdio.h>

const char* get_college_name(int college_code) {
    switch (college_code) {
        case 1: return "공과대학/SW융합대학";
        case 2: return "인문사회/경영대학";
        case 3: return "보건의료대학";
        default: return "알수없음";
    }
}

const char* get_major_name(int college_code, int major_code) {
    if (college_code == 1) {
        switch (major_code) {
            case 1: return "컴퓨터공학부";
            case 2: return "AI소프트웨어학과";
            case 3: return "전자공학과";
            case 4: return "기계공학과";
        }
    } else if (college_code == 2) {
        switch (major_code) {
            case 1: return "경영학과";
            case 2: return "미디어커뮤니케이션학과";
            case 3: return "상담심리학과";
        }
    } else if (college_code == 3) {
        switch (major_code) {
            case 1: return "간호학과";
            case 2: return "물리치료학과";
            case 3: return "치위생학과";
        }
    }
    return "알수없음";
}

void select_college_and_major(int *out_college, int *out_major) {
    int c_code, m_code;
    
    while(1) {
        printf("\n[단과대 선택]\n");
        printf("1. 공과대학/SW융합대학\n");
        printf("2. 인문사회/경영대학\n");
        printf("3. 보건의료대학\n");
        printf("선택: ");
        if (scanf("%d", &c_code) != 1) {
            while(getchar() != '\n');
            continue;
        }
        
        if (c_code < 1 || c_code > 3) {
            printf("잘못된 단과대 번호입니다.\n");
            continue;
        }
        
        printf("\n[전공 선택]\n");
        if (c_code == 1) {
            printf("1. 컴퓨터공학부\n2. AI소프트웨어학과\n3. 전자공학과\n4. 기계공학과\n");
            printf("선택: ");
            if (scanf("%d", &m_code) != 1) {
                while(getchar() != '\n');
                continue;
            }
            if (m_code < 1 || m_code > 4) {
                printf("잘못된 전공 번호입니다.\n");
                continue;
            }
        } else if (c_code == 2) {
            printf("1. 경영학과\n2. 미디어커뮤니케이션학과\n3. 상담심리학과\n");
            printf("선택: ");
            if (scanf("%d", &m_code) != 1) {
                while(getchar() != '\n');
                continue;
            }
            if (m_code < 1 || m_code > 3) {
                printf("잘못된 전공 번호입니다.\n");
                continue;
            }
        } else if (c_code == 3) {
            printf("1. 간호학과\n2. 물리치료학과\n3. 치위생학과\n");
            printf("선택: ");
            if (scanf("%d", &m_code) != 1) {
                while(getchar() != '\n');
                continue;
            }
            if (m_code < 1 || m_code > 3) {
                printf("잘못된 전공 번호입니다.\n");
                continue;
            }
        }
        break;
    }
    *out_college = c_code;
    *out_major = m_code;
}
