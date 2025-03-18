#include <library/cpp/getopt/small/opt.h>

#include <library/cpp/charset/codepage.h>
#include <library/cpp/charset/recyr.hh>
#include <library/cpp/charset/wide.h>
#include <util/generic/string.h>
#include <util/memory/tempbuf.h>
#include <util/system/tempfile.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _win_
#include <fcntl.h>
#include <io.h>
#endif // _win_

TString EncodeXMLString(const char *str);

void usage() {
    fprintf(stderr, "usage: recode [-e] [-i|-o output] ('cp_in'|-u|-l) 'cp_out' [<] (input|-) [ [>] output]\n");
}

int main(int argc, char *argv[]) {
    Opt getOpt(argc, argv, "ulehio:");
    int option, iflag = 0, oflag = 0;
    const char * file = nullptr;
    TString tempfile;
    enum case_map { no_case, upper, lower } flag = no_case;
    bool encodeEntities = false;
    ECharset in = CODES_UNKNOWN;
    ECharset out = CODES_UNKNOWN;

#ifdef _win_
    setmode(fileno(stdout), _O_BINARY);
#endif

    while ((option = getOpt()) != EOF) {
        switch (option) {
        case '?':
        case 'h':
        default:
            usage();
            exit(0);
            break;

        case 'o':
            ++oflag;
            if (iflag > 0 ) {
                usage();
                exit(1);
            }
            if (!freopen(getOpt.Arg, "w", stdout)) {
                perror(getOpt.Arg);
                exit(1);
            }

        case 'u':
            flag = upper;
            break;

        case 'l':
            flag = lower;
            break;
        case 'e':
            encodeEntities = true;
            break;

        case 'i':
            ++iflag;
            if (oflag > 0 ) {
                usage();
                exit(1);
            }
            tempfile = MakeTempName();
            break;

        }
    }
    if (argc - getOpt.Ind < (2 - (flag != no_case))) {
        usage();
        exit(0);
    }
    in = CharsetByName(argv[getOpt.Ind]);
    if (in == CODES_UNKNOWN) {
        fprintf(stderr, "unknown input charset: %s\n", argv[getOpt.Ind]);
        exit(1);
    }
    if (encodeEntities && (CharsetByName(argv[getOpt.Ind]) != CODES_YANDEX || flag != no_case)) {
        encodeEntities = false;
        fprintf(stderr, "Warning: ignoring '-e' option:\n");
        fprintf(stderr, "'-e' mode is not compatible with '-l' or '-u' and is only possible for yandex input coding\n");
    }
    getOpt.Ind++;
    if (flag == no_case) {
        out = CharsetByName(argv[getOpt.Ind]);
        if (out == CODES_UNKNOWN) {
            fprintf(stderr, "unknown output charset: %s\n", argv[getOpt.Ind]);
            exit(1);
        }
        getOpt.Ind++;
    }
    if (getOpt.Ind < argc) {
        file = argv[getOpt.Ind];
        if (strcmp(file, "-") && !freopen(file, "r", stdin)) {
            perror(file);
            exit(1);
        }
        getOpt.Ind++;
    }
    if (iflag) {
        if (!freopen(tempfile.data(), "w", stdout)) {
            perror(tempfile.data());
            exit(1);
        }
    } else {
        if (getOpt.Ind < argc) {
            if (!freopen(argv[getOpt.Ind], "w", stdout)) {
                perror(argv[getOpt.Ind]);
                exit(1);
            }
            getOpt.Ind++;
        }
    }
    if (getOpt.Ind < argc) {
        fprintf(stderr, "extra arguments ignored\n");
    }
    const unsigned in_size = 1024;
    char inbuf[in_size + 1]; // 1 byte for terminating zero -- I want it for EncodeXMLString
    int in_len = 0;

    const unsigned out_size = in_size * 4;
    TTempBuf tmpBuf(out_size);
    char* outbuf = tmpBuf.Data();
    size_t to_write = 0;
    size_t bytesRead = 0;

    bool bufferEmpty = true;
    TString entEncoded;
    const char *pe = nullptr;
    RECODE_RESULT res = RECODE_ERROR;
    while (1) {
        if (!encodeEntities || encodeEntities && bufferEmpty) {
            in_len = fread(inbuf, 1, in_size, stdin);
            if (in_len <= 0)
                break;
            inbuf[in_len] = '\0';
        }
        res = RECODE_ERROR;
        if (flag == no_case){
            if (encodeEntities) {
                if (bufferEmpty) {
                    entEncoded = EncodeXMLString(inbuf);
                    pe = entEncoded.c_str();
                    bufferEmpty = false;
                }
                if ((int)strlen(pe) > in_len) {
                    strncpy(inbuf, pe, in_len);
                    pe += in_len;
                } else {
                    in_len = strlen(pe);
                    strncpy(inbuf, pe, in_len);
                    bufferEmpty = true;
                }
            }
            res = Recode(in, out, inbuf,outbuf, in_len, out_size, bytesRead, to_write);
        }else{
            TUtf16String wide;
            CharToWide(TStringBuf(inbuf, in_len), wide, in);
            if (flag == upper) {
                wide.to_upper();
            } else { // (flag == lower)
                wide.to_lower();
            }
            TString string = WideToChar(wide.c_str(), wide.size(), in);
            strncpy(outbuf, string.c_str(), string.size());
            to_write = string.size();
            res = RECODE_OK;
        }
        if (res == RECODE_EOINPUT && !feof(stdin)){
            int bytesToUnget = in_len - bytesRead;
            if (1 <= bytesToUnget && bytesToUnget <= 3) {
                for (int i = bytesToUnget - 1; i >= 0; --i) {
                    ungetc(inbuf[bytesRead + i], stdin);
                }
                res = RECODE_OK;
            }
        }
        if (res == RECODE_OK) {
            fwrite(outbuf,1,to_write,stdout);
        } else {
            fprintf(stderr, "Broken file.\n");
            break;
        }
    }
    fclose(stdin);
    fclose(stdout);
    if (res == RECODE_OK) {
        if (iflag) {
            if (remove(file) != 0 || rename(tempfile.data(), file) != 0)
                exit(1);
        }
    } else {
        if (iflag)
            remove(tempfile.data());
        exit(1);
    }
    return 0;
}
