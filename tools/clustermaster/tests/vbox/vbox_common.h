#pragma once

enum ELogType {
    net = 0,
    fs  = 1,
    ps  = 2
};

int logme(ELogType log, const char *fmt, ...);
