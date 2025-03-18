#pragma once

#include <library/cpp/deprecated/datawork/conf/datawork_conf.h>

#include <library/cpp/getopt/opt.h>
#include <library/cpp/microbdb/microbdb.h>

#include <util/generic/string.h>
#include <util/system/compat.h>

#include <stdio.h>

#include <fcntl.h>

static inline TFile GetDuplicatedFile(FHANDLE handle) {
    TFileHandle fh(handle);
    FHANDLE dupHandle = fh.Duplicate();
    fh.Release();
    return TFile(dupHandle);
}

static inline TFile GetStandardInputFile() {
    FHANDLE handle;
#if defined(_win_)
    handle = GetStdHandle(STD_INPUT_HANDLE);
#elif defined(_unix_)
    handle = STDIN_FILENO;
#else
#error
#endif
    return GetDuplicatedFile(handle);
}

static inline TFile GetStandardOutputFile() {
    FHANDLE handle;
#if defined(_win_)
    handle = GetStdHandle(STD_OUTPUT_HANDLE);
#elif defined(_unix_)
    handle = STDOUT_FILENO;
#else
#error
#endif
    return GetDuplicatedFile(handle);
}

static inline int OpenInputFile(const char* nin, TFile& ifile) {
    if (!nin || !*nin || !strcmp(nin, "-"))
        ifile = GetStandardInputFile();
    else
        ifile = TFile(nin, RdOnly);

    return 0;
}

static inline int OpenOutputFile(const char* nout, TFile& ofile) {
    if (!nout || !*nout || !strcmp(nout, "-"))
        ofile = GetStandardOutputFile();
    else {
        if (unlink(nout) && errno != ENOENT) {
            ythrow yexception() << "Unlink \"" << nout << "\" failed with errno: " << errno;
        }
        ofile = TFile(nout, WrOnly | CreateAlways | ARW | AWOther);
    }

    return 0;
}

static inline int OpenFiles(const char* nin, const char* nout, TFile& ifile, TFile& ofile) {
    int r = 0;
    if ((r = OpenInputFile(nin, ifile)) != 0)
        return r;

    r = OpenOutputFile(nout, ofile);
    return r;
}

template <typename T, size_t BASE>
T ParseRawInt(const TString& stroka) {
    if (stroka.empty())
        ythrow yexception() << "no digits";
    T r = 0;
    for (size_t i = 0; i < stroka.size(); ++i) {
        char c = stroka[i];
        unsigned char digit = 255;
        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (c >= 'a' && c <= 'z')
            digit = c + 10 - 'a';
        else if (c >= 'A' && c <= 'Z')
            digit = c + 10 - 'A';
        if (digit > BASE)
            ythrow yexception() << "unexpected digit";
        r = r * BASE + digit;
    }
    return r;
}

template <typename T>
T ParseInt(const TString& stroka) {
    if (stroka.StartsWith("0x"))
        return ParseRawInt<T, 16>(stroka.substr(2));
    else if (stroka.StartsWith('0'))
        return ParseRawInt<T, 8>(stroka.substr(8));
    else
        return ParseRawInt<T, 10>(stroka);
}
