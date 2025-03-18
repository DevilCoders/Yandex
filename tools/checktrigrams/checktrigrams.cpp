#include <dict/trigrams/trigrams.h>

#include <library/cpp/getopt/opt.h>

#include <util/generic/string.h>
#include <util/string/split.h>

#include <iostream>
using namespace std;

void usage(){
    cerr<<"usage: checktrigrams -(l|c)\n";
}
int main(int argc, char *argv[]) {
    Opt opt(argc, argv, "lch");
    int isLat = 0;
    int isCyr = 0;
    int c;
    while ((c = opt.Get()) != -1) {
        switch (c) {
        case 'l':
            isLat = 1;
            break;
        case 'c':
            isCyr = 1;
            break;
        default:
        case '?':
        case 'h':
            usage();
            return 1;
        }
    }
    if (isCyr == isLat) {
        usage();
        return 1;
    }
    Trigrams &trigrams = RussianTrigrams;
    if (isLat == 1)
        trigrams = EnglishTrigrams;

    TString line;
    while(Cin.ReadLine(line)) {
        for (auto word_it : StringSplitter(line).SplitByFunc(::isspace)) {
            TString word{word_it.Token()};
            int res = trigrams.checkWord(word.c_str());
            cout<<word.c_str()<<":";
            int res_num = 0;
            if (res & Trigrams::TRI_BAD_DIGRAM){
                if (res_num > 0) cout<<"|";
                cout<<"BAD_DIGRAM";
                res_num++;
            }
            if (res & Trigrams::TRI_BAD_PREF){
                if (res_num > 0) cout<<"|";
                cout<<"BAD_PREF";
                res_num++;
            }
            if (res & Trigrams::TRI_BAD_SUF){
                if (res_num > 0) cout<<"|";
                cout<<"BAD_SUF";
                res_num++;
            }
            if (res & Trigrams::TRI_BAD_MID){
                if (res_num > 0) cout<<"|";
                cout<<"BAD_MID";
                res_num++;
            }
            if (res & Trigrams::TRI_BAD_CHAR){
                if (res_num == 0) //only if no other errors
                    cout<<"BAD_CHAR";
            }
            if (res == 0)
                cout<<"OK";
            cout<<endl;
        }
    }
    return 0;
}
