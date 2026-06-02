#ifndef DECOLATE_H
#define DECOLATE_H

void print_main_menu();
void print_user_menu();
void wait_enter_to_clear();

int utf8_char_width(const char *s);
void print_fixed_utf8(const char *s, int width);

#endif // DECOLATE_H