/*
 * !Note
 *
 * Default user and password are:
 * User: ftpclient
 * Pass: 123456
 */
#ifndef _LOGIN_H
#define _LOGIN_H

#include "ftp.h"

#define TRY_TIMES		3

void initftp(void);
Status config(void);
Status load_configure(const char *configure_file);
Status get_word_form_str(char *str, char line[], int maxlen);
Status login(void);
void atlast(void);

#endif