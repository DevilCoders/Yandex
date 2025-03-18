#include <tools/segutils/tests/tests_common/dater_test.h>

int main(int argc, const char** argv) {
    using namespace NSegutils;
    TDaterTest test;
    test.Init(argc, argv);
    test.RunTest();
}
