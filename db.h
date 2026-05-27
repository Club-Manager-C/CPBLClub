#ifndef DB_H
#define DB_H

#include <mysql.h>
#include <windows.h>

void init_db(MYSQL **conn);
int check_login(MYSQL *conn, const char *id, const char *pw);
int check_id_duplicate(MYSQL *conn, const char *id);
int check_nickname_duplicate(MYSQL *conn, const char *nickname);
int check_student_id_duplicate(MYSQL *conn, long long student_id);
int register_user(MYSQL *conn, const char *id, const char *pw,
                  const char *nickname, long long student_id, const char *name,
                  const char *major, const char *phone);
void close_db(MYSQL *conn);
int insert_message(MYSQL *conn, const char *user_id, const char *content);
int get_user_idx_by_id(MYSQL *conn, const char *user_id);
int is_user_club_leader(MYSQL *conn, const char *user_id);

// ── 게시글 ──────────────────────────────────────
void get_my_posts(MYSQL *conn, const char *user_id);
int update_post(MYSQL *conn, int post_id, const char *user_id,
                const char *new_content);
void get_posts_by_category(MYSQL *conn, int category_id);
int insert_post(MYSQL *conn, const char *user_id, int category_id, const char *title, const char *content);
int delete_post(MYSQL *conn, int post_id, const char *user_id);

// ── 댓글 ────────────────────────────────────────
void get_my_comments(MYSQL *conn, const char *user_id);
int update_comment(MYSQL *conn, int comment_id, const char *user_id,
                   const char *new_content);
int insert_comment(MYSQL *conn, int post_id, const char *user_id, const char *content, int parent_comment_id);
int insert_comment_like(MYSQL *conn, int comment_id, const char *user_id);
int get_comment_likes_count(MYSQL *conn, int comment_id);
int has_user_liked_comment(MYSQL *conn, int comment_id, const char *user_id);


// ── 시간표 ──────────────────────────────────────
void get_my_schedule(MYSQL *conn, const char *user_id);
int add_schedule(MYSQL *conn, const char *user_id, const char *day,
                 const char *start, const char *end, const char *subject,
                 const char *location);
int delete_schedule(MYSQL *conn, int schedule_id, const char *user_id);
// ── 동아리 관리 (동아리장 전용) ──────────────────────────
int get_user_owned_club(MYSQL *conn, const char *user_id);
int verify_club_owner(MYSQL *conn, int club_id, const char *logged_id);
int kick_club_member(MYSQL *conn, int club_id, const char *owner_id, const char *target_student_id);
int create_promotion_post(MYSQL *conn, int club_id, const char *owner_id, const char *title, const char *content);
int transfer_club_ownership(MYSQL *conn, int club_id, const char *current_owner_id, const char *target_student_id);
int update_club_info(MYSQL *conn, int club_id, const char *owner_id, const char *new_name, const char *new_desc);

#endif // DB_H
