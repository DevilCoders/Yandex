#pragma once

#include <util/generic/cast.h>
#include <util/generic/hash.h>
#include <util/datetime/base.h>
#include <util/stream/mem.h>
#include <util/ysaveload.h>
#include <util/stream/str.h>

#include <library/cpp/deprecated/datawork/conf/datawork_conf.h>

#include <array>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

//*********************************************************************
//template <typename field> int ReadField(field &, const char *&);
//*********************************************************************

#ifdef _MSC_VER
#define strtoll _strtoi64
#define strtoull _strtoui64
#else
long long strtoll(const char* nptr, char** endptr, int base);
unsigned long long strtoull(const char* nptr, char** endptr, int base);
#endif

static inline int ReadField(double& field, char const*& ptr) {
    char* endptr;
    field = strtod(ptr, &endptr);
    if (errno == ERANGE || endptr == ptr)
        return 1;
    ptr = endptr;
    return 0;
}
static inline int ReadField(float& field, char const*& ptr) {
    char* endptr;
#ifdef _MSC_VER
    field = (float)strtod(ptr, &endptr);
#else
    field = strtof(ptr, &endptr);
#endif
    if (errno == ERANGE || endptr == ptr)
        return 1;
    ptr = endptr;
    return 0;
}

static inline int ReadField(long long& field, char const*& ptr) {
    if (!isdigit(*ptr) && *ptr != '-')
        return 1;
    field = strtoll(ptr, &(char*&)ptr, 10);
    return (errno == ERANGE) ? 1 : 0;
}

static inline int ReadField(unsigned long long& field, char const*& ptr) {
    if (!isdigit(*ptr))
        return 1;
    field = strtoull(ptr, &(char*&)ptr, 10);
    return (errno == ERANGE) ? 1 : 0;
}

static inline int ReadField(long& field, char const*& ptr) {
    if (!isdigit(*ptr) && *ptr != '-')
        return 1;
    field = strtol(ptr, &(char*&)ptr, 10);
    return (errno == ERANGE) ? 1 : 0;
}

static inline int ReadField(unsigned long& field, char const*& ptr) {
    if (!isdigit(*ptr))
        return 1;
    field = strtoul(ptr, &(char*&)ptr, 10);
    return (errno == ERANGE) ? 1 : 0;
}

static inline int ReadField(int& field, char const*& ptr) {
    long f;
    if (ReadField(f, ptr) || f > INT_MAX || f < INT_MIN)
        return 1;
    field = (int)f;
    return 0;
}

static inline int ReadField(unsigned int& field, char const*& ptr) {
    unsigned long f;
    if (ReadField(f, ptr) || f > UINT_MAX)
        return 1;
    field = (unsigned int)f;
    return 0;
}

static inline int ReadField(short& field, char const*& ptr) {
    int f;
    if (ReadField(f, ptr) || f > SHRT_MAX || f < SHRT_MIN)
        return 1;
    field = (short)f;
    return 0;
}

static inline int ReadField(unsigned short& field, char const*& ptr) {
    unsigned int f;
    if (ReadField(f, ptr) || f > USHRT_MAX)
        return 1;
    field = (unsigned short)f;
    return 0;
}

static inline int ReadField(signed char& field, char const*& ptr) {
    int f;
    if (ReadField(f, ptr) || f > SCHAR_MAX || f < SCHAR_MIN)
        return 1;
    field = (signed char)f;
    return 0;
}

static inline int ReadField(unsigned char& field, char const*& ptr) {
    unsigned int f;
    if (ReadField(f, ptr) || f > UCHAR_MAX)
        return 1;
    field = (unsigned char)f;
    return 0;
}

template <size_t N>
static inline int ReadField(std::array<char, N>& field, char const*& ptr) {
    const char* pend = strchr(ptr, '\t');
    if (!pend && !(pend = strchr(ptr, '\n')))
        return 1;
    if (ptr == pend || (size_t)(pend - ptr + 1) > N)
        return 1;
    memcpy(field.data(), ptr, pend - ptr);
    field[pend - ptr] = 0;
    ptr = pend;
    return 0;
}

//*********************************************************************
//template <typename field> void WriteField(const field &, char *&);
//*********************************************************************

static inline void WriteField(const double field, char*& ptr) {
    ptr += sprintf(ptr, "%e", field);
}

const static int LONG_LONG_BUF_LEN = (4 * sizeof(long long) + 10);
static inline void WriteField(const unsigned long long field, char*& ptr) {
    char buf[LONG_LONG_BUF_LEN];
    char* buf_ptr = buf + LONG_LONG_BUF_LEN;
    unsigned long long f = field;
    while (f > UINT_MAX) {
        *--buf_ptr = char((f % 10) + '0');
        f /= 10;
    }
    unsigned int f1 = (unsigned int)f;
    do {
        *--buf_ptr = char((f1 % 10) + '0');
        f1 /= 10;
    } while (f1);
    memcpy(ptr, buf_ptr, buf + LONG_LONG_BUF_LEN - buf_ptr);
    ptr += buf + LONG_LONG_BUF_LEN - buf_ptr;
}

static inline void WriteField(const long long field, char*& ptr) {
    if (field < 0) {
        *ptr++ = '-';
        WriteField((const unsigned long long)-field, ptr);
    } else {
        WriteField((const unsigned long long)field, ptr);
    }
}

static inline void WriteField(const unsigned long field, char*& ptr) {
    char buf[LONG_LONG_BUF_LEN];
    char* buf_ptr = buf + LONG_LONG_BUF_LEN;
    unsigned long f = field;
    do {
        *--buf_ptr = char((f % 10) + '0');
        f /= 10;
    } while (f);
    memcpy(ptr, buf_ptr, buf + LONG_LONG_BUF_LEN - buf_ptr);
    ptr += buf + LONG_LONG_BUF_LEN - buf_ptr;
}

static inline void WriteField(const long field, char*& ptr) {
    if (field < 0) {
        *ptr++ = '-';
        WriteField((const unsigned long)(-field), ptr);
    } else {
        WriteField((const unsigned long)field, ptr);
    }
}

static inline void WriteField(const unsigned int field, char*& ptr) {
    char buf[LONG_LONG_BUF_LEN];
    char* buf_ptr = buf + LONG_LONG_BUF_LEN;
    unsigned int f = field;
    do {
        *--buf_ptr = char((f % 10) + '0');
        f /= 10;
    } while (f);
    memcpy(ptr, buf_ptr, buf + LONG_LONG_BUF_LEN - buf_ptr);
    ptr += buf + LONG_LONG_BUF_LEN - buf_ptr;
}

static inline void WriteField(const int field, char*& ptr) {
    if (field < 0) {
        *ptr++ = '-';
        WriteField((const unsigned int)(-field), ptr);
    } else {
        WriteField((const unsigned int)field, ptr);
    }
}

static inline void WriteField(const unsigned short field, char*& ptr) {
    WriteField((const unsigned int)field, ptr);
}

static inline void WriteField(const short field, char*& ptr) {
    WriteField((const int)field, ptr);
}

static inline void WriteField(const unsigned char field, char*& ptr) {
    WriteField((const unsigned int)field, ptr);
}

static inline void WriteField(const signed char field, char*& ptr) {
    WriteField((const int)field, ptr);
}

template <size_t N>
static inline void WriteField(const std::array<char, N>& field, char*& ptr) {
    size_t len = strlen(field.data());
    memcpy(ptr, field.data(), len);
    ptr += len;
}
