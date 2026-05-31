#ifndef BOARD_H
#define BOARD_H

#include "db.h"

// 가입 신청 상태
typedef enum JoinRequestStatus {
    PENDING = 0,
    APPROVED = 1,
    REJECTED = 2
} JoinRequestStatus;

// 가입 신청 구조체
typedef struct JoinRequest {
    int request_id;
    int club_id;
    int user_idx;
    char introduction[500];
    JoinRequestStatus status;
} JoinRequest;

// 댓글 구조체 (좋아요 수 필드 추가)
typedef struct Comment {
    int comment_id;
    int post_id;              // 소속 게시글 매핑 (Foreign Key 역할)
    char writer_id[50];       // 작성자 아이디
    char content[500];
    int likes;                // 댓글 좋아요 수
    int parent_comment_id;    // 대댓글 매핑 (최상위는 0)
    struct Comment *next;     // 다음 댓글을 가리키는 포인터
} Comment;

// 게시글 구조체
typedef struct Post {
    int post_id;
    int club_id;              // 작성된 동아리 (가입 신청 시 필요)
    char title[255];
    char content[1000];
    int view_count;
    int like_count;           // 게시글 좋아요 수
    Comment *comments_head;   // [매핑] 해당 게시글에 달린 댓글 리스트의 시작점 (1:N)
} Post;

int like_post(MYSQL *conn, int post_id, const char *logged_id);
void apply_for_club(MYSQL *conn, int club_id, const char *logged_id);

void show_board_menu(MYSQL *conn, int category_id, const char *logged_id);

// 동아리 홍보 게시판 (카테고리 ID 1)
void promo_board(MYSQL *conn, const char *logged_id);

// 전공 동아리 게시판 (카테고리 ID 2)
void major_club_board(MYSQL *conn, const char *logged_id);

// 게시글 상세 메뉴 (공유용)
void view_post_detail_menu(MYSQL *conn, int post_id, const char *logged_id);
int is_string_whitespace_only(const char *str);

#endif // BOARD_H
