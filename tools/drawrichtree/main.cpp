#include <kernel/drawrichtree/drawrichtree.h>

#include <library/cpp/charset/codepage.h>
#include <library/cpp/getopt/opt.h>

#include <util/generic/yexception.h>

int main(int argc, char* argv[])
{
    try {
        ECharset encoding = CODES_UTF8;
        Opt opt(argc, argv, "e:q:");
        int c;
        TString qtree;
        while ((c = opt.Get()) != EOF) {
            switch (c){
            case 'e':
                encoding = CharsetByName(opt.Arg);
                break;
            case 'q':
                qtree = opt.Arg;
                break;
            case '?':
            default:
                Cerr << "usage:" << Endl;
                Cerr << "drawrichtree [-e encoding] [-q qtree]" << Endl;
                return EXIT_FAILURE;
            }
        }

        if (!qtree.empty()) {
            NDrawRichTree::GenerateGraphViz(qtree, Cout, encoding);
        } else {
            NDrawRichTree::GenerateGraphViz(Cin, Cout, encoding);
        }

        if (opt.Ind < argc) {
            Cerr << "extra arguments ignored: " << argv[opt.Ind] << "..." << Endl;
        }

        return 0;

    } catch (const yexception& e) {
        Cerr << e.what() << Endl;
    }

    return 1;
}
