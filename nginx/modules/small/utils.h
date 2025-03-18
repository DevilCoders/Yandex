#pragma once
#include <ngx_config.h>
#include <ngx_core.h>

int64_t ngx_hextoi64(u_char *line, size_t n);
int64_t ngx_atoi64(u_char *line, size_t n);
