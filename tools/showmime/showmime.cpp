#include <library/cpp/mime/detect/detectmime.h>
#include <library/cpp/mime/types/mime.h>
#include <library/cpp/getopt/opt.h>

#include <util/system/defaults.h>

void PrintUsage(const char *name) {
    fprintf(stderr, "Detects document mime-type based on http-response headers and document contents\n");
    fprintf(stderr, "Usage: %s [-m mimetype] [-v] [-e] document\n", name);
    fprintf(stderr, "Options:\n"
                    "   -m mimetype       mime-type from server http-response\n"
                    "   -v                be verbose\n"
                    "   -e                print convertible to MimeTypes string\n"
                    "Mime-types:\n    "
    );
    for (unsigned i = 1; i < MIME_MAX; i++) {
        fprintf(stderr, "%s%s%s", i == 1 ? "" : ", ", i % 10 ? "" : "\n    ", MimeNames[i]);
    }
    fprintf(stderr, "\n");
}

MimeTypes GetTypeFromStr(const char *mime_str) {
    for (unsigned i = 1; i < MIME_MAX; i++)
        if (strcmp(mime_str, MimeNames[i]) == 0)
            return (MimeTypes)i;
    return MIME_UNKNOWN;
}

const size_t N = 1;

int main(int argc, char **argv) {
    MimeTypes mime_server = MIME_UNKNOWN;
    bool verbose = false;
    bool mimeEnum = false;

    int optlet;
    Opt opt(argc, argv, "vm:e");
    while ((optlet = opt.Get()) != EOF) {
        switch (optlet) {
        case 'm':
            mime_server = GetTypeFromStr(opt.Arg);
            if (mime_server == MIME_UNKNOWN) {
                fprintf(stderr, "unknown mime: \"%s\"\n", opt.Arg);
                PrintUsage(argv[0]);
                exit(1);
            }
            break;
        case 'v':
            verbose = true;
            break;
        case 'e':
            mimeEnum = true;
            break;
        default:
            PrintUsage(argv[0]);
            exit(1);
        }
    }

    if (opt.Ind + 1 != argc) {
        PrintUsage(argv[0]);
        exit(1);
    }

    unsigned char buf[N];
    FILE *f = fopen(argv[opt.Ind], "rb");
    if (! f) {
        printf("File not found: \"%s\"\n", argv[1]);
        return 1;
    }

    TMimeDetector det;

    size_t s;
    size_t pos = 0;
    do {
        s = fread(buf, 1, N, f);
        pos += s;
        if (s == 0) break;
    } while (det.Detect(buf, s));

    if (verbose)
        det.PrintByteStat();

    if (!mimeEnum) {
        printf("%s on position %" PRISZT "\n", MimeNames[det.Mime(mime_server)], pos);
    } else {
        const char* mimeStr = strByMime(det.Mime(mime_server));
        if (mimeStr == nullptr) {
            printf("strByMime failed");
            return 1;
        }
        printf("%s\n", mimeStr);
    }

    fclose(f);

    return 0;
}
