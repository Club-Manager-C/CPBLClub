#ifndef DB_H
#define DB_H

#include <windows.h>
#include <mysql.h>

void init_db(MYSQL **conn);
int check_login(MYSQL *conn, const char *id, const char *pw);
int check_id_duplicate(MYSQL *conn, const char *id);
int check_nickname_duplicate(MYSQL *conn, const char *nickname);
int check_student_id_duplicate(MYSQL *conn, long long student_id);
int register_user(MYSQL *conn, const char *id, const char *pw,
                  const char *nickname, long long student_id,
                  const char *name, const char *major, const char *phone);
void close_db(MYSQL *conn);

// ── 게시글 ──────────────────────────────────────
void get_my_posts(MYSQL *conn, const char *user_id);
int  update_post(MYSQL *conn, int post_id, const char *user_id,
                 const char *new_content);
void get_posts_by_category(MYSQL *conn, int category_id);

// ── 댓글 ────────────────────────────────────────
void get_my_comments(MYSQL *conn, const char *user_id);
int  update_comment(MYSQL *conn, int comment_id, const char *user_id,
                    const char *new_content);

// ── 시간표 ──────────────────────────────────────
void get_my_schedule(MYSQL *conn, const char *user_id);
int  add_schedule(MYSQL *conn, const char *user_id, const char *day,
                  const char *start, const char *end,
                  const char *subject, const char *location);
int  delete_schedule(MYSQL *conn, int schedule_id, const char *user_id);

#endif // DB_H
