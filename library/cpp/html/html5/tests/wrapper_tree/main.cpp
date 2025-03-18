#include <library/cpp/html/html5/tests/common/test.h>
#include <library/cpp/html/html5/tests/common/read.h>
#include <library/cpp/html/html5/tests/tree/treebuild.h>

#include <util/stream/file.h>
#include <util/stream/str.h>

class TTreePrinter: public ITest {
public:
    TTreePrinter(int argc, const char** argv)
        : ITest(argc, argv)
    {
    }

    ~TTreePrinter() {
    }

private:
    void ProcessTestsFile(const TString& inputPath) override {
        TFileInput input(inputPath);

        while (1) {
            try {
                TString docStr;
                ReadOneTestChunk(&input, &docStr);
                ProcessSingleDoc(docStr);
                Cout << Endl;
            } catch (yexception&) {
                break;
            }
        }
    }

    void ProcessSingleDoc(const TString& input) override {
        PrintChunkTree(input, Cout);
    }
};

int main(int argc, const char** argv) {
    TTreePrinter(argc, argv).Run();

    return 0;
}
