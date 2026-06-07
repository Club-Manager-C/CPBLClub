#ifndef DECORATE_H
#define DECORATE_H

void print_main_menu();
void print_user_menu();
void wait_enter_and_clear(const char *message);
int utf8_char_width(const char *s);
int utf8_char_len(const char *s);
void print_fixed_utf8(const char *s, int width);

#endif // DECORATE_H