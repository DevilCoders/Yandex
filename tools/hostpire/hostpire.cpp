#include <cstdlib>
#include <unistd.h>
#include <iostream>

#include <library/cpp/charset/ci_string.h>
#include <util/generic/string.h>
#include <util/generic/map.h>
#include <library/cpp/regex/pire/pire.h>
#include <library/cpp/regex/pire/regexp.h>

void usage() {
    std::cerr << "Usage:" << std::endl;
    std::cerr << "    " << "applyHostPire  fnHostRules" << std::endl << std::endl;
    std::cerr << "    The module reads from input tab-delimited strings, which begins from url, number of fields may be any." << std::endl;
    std::cerr << "    ATTN: treats only hosts and paths and does not scheemes and passwords!!!" << std::endl;
    std::cerr << "    Assumes that first (if any) field is url" << std::endl;
    std::cerr << "    Assumes that the input is sorted by host (this assumption has an influence on speed only)" << std::endl;
    std::cerr << "    Writes to output strings, which paths are matched to RE, specified to host" << std::endl;
    std::cerr << "    RE for hosts paths are specified in file which name set in argv[1]; file format: hostname <tab> re " << std::endl;
}

char hostBuf[5000];
char oldHost[5000];
char strBuf[500000];

int main(int argc, char** argv) {

    if (argc != 2) {
        usage();
        return 1;
    }

    FILE* f = fopen(argv[1], "r");
    if (!f) {
        std::cerr << "Cannot open " << argv[1] << ": " <<  strerror(errno) << std::endl;
        return errno;
    }

    typedef TMap<TCiString, NRegExp::TFsm *> THostsRules;
    THostsRules hRules;
    NRegExp::TFsm *aRule = nullptr;

    while (fscanf(f, "%s\t%s", hostBuf, strBuf) != EOF) {
        try {
            if (*hostBuf) {
                hRules[hostBuf] = new NRegExp::TFsm(strBuf);
        }
        } catch (yexception e) {
            std::cerr << "exception caught for host " << hostBuf << " rule '" << strBuf << "': " << e.what() << std::endl;
            delete hRules[hostBuf];
            hRules[hostBuf] = nullptr;
        }
    }

    FILE* fIn = fopen("/dev/stdin", "r");
    if (!fIn) {
        std::cerr << "Cannot open " << "/dev/stdin" << ": " <<  strerror(errno) << std::endl;
        return errno;
    }

    oldHost[0] = 0;
    THostsRules::const_iterator aRuleIter;

    while (fgets(strBuf, sizeof(strBuf), fIn)) {
        char *str = (char *)strBuf;

        strsep(&str, "\r\n");               // remove <cr><lf>

        str = (char *)strBuf;
        char *path = strsep(&str, "\t");    // now path is url
        char *aHost = strsep(&path, "/");   // and now path is path

        if (strncmp(aHost, oldHost, 5000)) {
             aRuleIter = hRules.find(aHost);
             if (aRuleIter != hRules.end()) {
                 aRule = aRuleIter->second;
             } else {
                 aRule = nullptr;
             }
             oldHost[0] = 0;
             strncat(oldHost, aHost, 5000);
        }

        if (aRule && NRegExp::TMatcher(*aRule).Match(path ? path : "").Final()) {
            std::cout << aHost;
            if (path) {
                std::cout << "/" << path;
            }
            if (str) {
                 std::cout << "\t" << str;
            }
            std::cout << std::endl;
        }
    }

    for (THostsRules::iterator aRI = hRules.begin(); aRI != hRules.end(); ++aRI) {
        delete aRI->second;
    }

}
