#include "batch_replacer.h"

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>

#include <string>


using namespace NFacts;

Y_UNIT_TEST_SUITE(TBatchReplacer_Construct) {

Y_UNIT_TEST(Successful) {
    const TString config = R"ELEPHANT({
            "my_batch_a": [
                {
                },
                {
                    "nop": true
                },
                {
                    "shrink_spaces": true
                }
            ],
            "my_batch_b": [
                {
                    "pattern": ":)",
                    "global": true
                },
                {
                    "regex": "\\. +\\.([^0-9a-zA-Zа-яА-Я_]|$)",
                    "replacement": "..\\1",
                    "repeated": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
}

Y_UNIT_TEST(EmptyReplacer) {
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer("{}")));
}

Y_UNIT_TEST(ErrorInvalidSchemeNotArray) {
    const TString config = R"ELEPHANT({
            "my_batch": 1
        })ELEPHANT";
    UNIT_ASSERT_EXCEPTION_CONTAINS(TBatchReplacer(config),
        yexception, "Error: path '/__root__/my_batch' is not an array");
}

Y_UNIT_TEST(ErrorInvalidSchemeDictInsteadOfArray) {
    const TString config = R"ELEPHANT({
            "my_batch": {
                "pattern": "a",
                "replacement": "b"
            }
        })ELEPHANT";
    UNIT_ASSERT_EXCEPTION_CONTAINS(TBatchReplacer(config),
        yexception, "Error: path '/__root__/my_batch' is not an array");
}

Y_UNIT_TEST(ErrorInvalidSchemeNotDict) {
    const TString config = R"ELEPHANT({
            "my_batch": [1, 2]
        })ELEPHANT";
    UNIT_ASSERT_EXCEPTION_CONTAINS(TBatchReplacer(config),
        yexception, "Error: path '/__root__/my_batch/0' is not a dict");
}

Y_UNIT_TEST(ErrorInvalidJson) {
    const TString config = R"ELEPHANT({
            "my_batch": [],
            "my_batch": []
        })ELEPHANT";
    UNIT_ASSERT_EXCEPTION_CONTAINS(TBatchReplacer(config),
        NSc::TSchemeParseException, "JSON error at offset 52 (Offset: 52, Code: 16, Error: Terminate parsing due to Handler error.)");
}

Y_UNIT_TEST(ErrorInvalidRegex) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "regex": "(?<= *) ",
                    "replacement": ""
                }
            ]
        })ELEPHANT";
    UNIT_ASSERT_EXCEPTION_CONTAINS(TBatchReplacer(config),
        yexception, "Invalid regex r'(?<= *) ' in batch 'my_batch'.");
}

}  // Y_UNIT_TEST_SUITE(TBatchReplacer_Construct)


Y_UNIT_TEST_SUITE(TBatchReplacer_Process) {

Y_UNIT_TEST(Successful) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "a",
                    "replacement": "b"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "b");
}

Y_UNIT_TEST(EmptyBatch) {
    const TString config = R"ELEPHANT({
            "my_batch": []
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "a");
}

Y_UNIT_TEST(BatchA) {
    const TString config = R"ELEPHANT({
            "my_batch_a": [
                {
                    "pattern": "a",
                    "replacement": "A"
                }
            ],
            "my_batch_b": [
                {
                    "pattern": "b",
                    "replacement": "B"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "ab";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch_a"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "Ab");
}

Y_UNIT_TEST(BatchB) {
    const TString config = R"ELEPHANT({
            "my_batch_a": [
                {
                    "pattern": "a",
                    "replacement": "A"
                }
            ],
            "my_batch_b": [
                {
                    "pattern": "b",
                    "replacement": "B"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "ab";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch_b"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "aB");
}

Y_UNIT_TEST(ErrorBatchNotFound) {
    const TString config = R"ELEPHANT({
            "my_batch": []
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    UNIT_ASSERT_EXCEPTION_CONTAINS([&](){ std::string text; batchReplacer->Process(&text, "other_batch"); }(),
        yexception, "Batch 'other_batch' not found.");
}

Y_UNIT_TEST(MultipleOperations) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "regex": "a",
                    "replacement": "  ",
                    "global": true
                },
                {
                    "shrink_spaces": true
                },
                {
                    "nop": true,
                    "regex": "xxx xxx",
                    "replacement": "z",
                    "repeated": true
                },
                {
                    "regex": "xxx xxx",
                    "replacement": "w",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "xxxaxxxaxxxaxxx";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "xxx(a)xxx(a)xxx(a)xxx" -> "xxx(  )xxx(  )xxx(  )xxx"
    // "xxx  xxx  xxx  xxx"    -> "xxx xxx xxx xxx"
    // "(xxx xxx) (xxx xxx)"   -> "(w) (w)"
    UNIT_ASSERT(text == "w w");
}

Y_UNIT_TEST(ModifiedByFirst) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "a",
                    "replacement": "A"
                },
                {
                    "pattern": "b",
                    "replacement": "B"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "A");
}

Y_UNIT_TEST(ModifiedBySecond) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "a",
                    "replacement": "A"
                },
                {
                    "pattern": "b",
                    "replacement": "B"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "b";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "B");
}

Y_UNIT_TEST(ModifiedByAll) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "a",
                    "replacement": "A"
                },
                {
                    "pattern": "b",
                    "replacement": "B"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "ab";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "AB");
}

Y_UNIT_TEST(ModifiedByNeither) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "a",
                    "replacement": "A"
                },
                {
                    "pattern": "b",
                    "replacement": "B"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "z";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "z");
}

}  // Y_UNIT_TEST_SUITE(TBatchReplacer_Process)


Y_UNIT_TEST_SUITE(TNoOperation_Process) {

Y_UNIT_TEST(Implicit) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {}
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "a");
}

Y_UNIT_TEST(Explicit) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "nop": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "a");
}

Y_UNIT_TEST(OvercomesShrinkSpaces) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "nop": true,
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a   a";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "a   a");
}

Y_UNIT_TEST(OvercomesReplaceByPattern) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "nop": true,
                    "pattern": "a",
                    "replacement": "b"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "a");
}

Y_UNIT_TEST(OvercomesReplaceByRegex) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "nop": true,
                    "regex": "a",
                    "replacement": "b"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "a");
}

Y_UNIT_TEST(SwitchedOff) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "nop": false,
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a   a";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "a a");
}

}  // Y_UNIT_TEST_SUITE(TNoOperation_Process)


Y_UNIT_TEST_SUITE(TShrinkSpaces_Process) {

Y_UNIT_TEST(Successful) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a   a";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "a a");
}

Y_UNIT_TEST(EmptyText) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "");
}

Y_UNIT_TEST(Inner1) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a b";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "a b");
}

Y_UNIT_TEST(Inner2) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a  b";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "a b");
}

Y_UNIT_TEST(Inner3) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a   b";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "a b");
}

Y_UNIT_TEST(InnerN) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a    b";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "a b");
}

Y_UNIT_TEST(InnerMulty) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a  b cd   e    fgh i  j";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "a b cd e fgh i j");
}

Y_UNIT_TEST(Leading1) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = " a";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "a");
}

Y_UNIT_TEST(Leading2) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "  a";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "a");
}

Y_UNIT_TEST(LeadingN) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "   a";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "a");
}

Y_UNIT_TEST(Leading1AndInner) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = " a  b cd   e    fgh i  j";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "a b cd e fgh i j");
}

Y_UNIT_TEST(Leading2AndInner) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "  a  b cd   e    fgh i  j";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "a b cd e fgh i j");
}

Y_UNIT_TEST(Trailing1) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a ";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "a");
}

Y_UNIT_TEST(Trailing2) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a  ";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "a");
}

Y_UNIT_TEST(TrailingN) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a   ";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "a");
}

Y_UNIT_TEST(Trailing1AndInner) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a  b cd   e    fgh i  j ";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "a b cd e fgh i j");
}

Y_UNIT_TEST(Trailing2AndInner) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "a  b cd   e    fgh i  j  ";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "a b cd e fgh i j");
}

Y_UNIT_TEST(Complex) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "   a  b cd   e    fgh i  j    ";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "a b cd e fgh i j");
}

Y_UNIT_TEST(Blank1) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = " ";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "");
}

Y_UNIT_TEST(Blank2) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "  ";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "");
}

Y_UNIT_TEST(BlankN) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "shrink_spaces": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "   ";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "");
}

}  // Y_UNIT_TEST_SUITE(TShrinkSpaces_Process)


Y_UNIT_TEST_SUITE(TReplaceByPattern_Process) {

Y_UNIT_TEST(Replace) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "a",
                    "replacement": "A"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqraxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrAxyz");
}

Y_UNIT_TEST(Remove) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "a"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqraxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrxyz");
}

Y_UNIT_TEST(Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "aba",
                    "replacement": "a",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abababa";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "(aba)b(aba)" -> "(a)b(a)"
    UNIT_ASSERT(text == "aba");
}

Y_UNIT_TEST(Repeated) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "aba",
                    "replacement": "a",
                    "repeated": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abababa";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "(aba)baba" -> "(a)baba"
    // "(aba)ba"   -> "(a)ba"
    // "(aba)"     -> "(a)"
    UNIT_ASSERT(text == "a");
}

Y_UNIT_TEST(First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "aba",
                    "replacement": "a"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abababa";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "(aba)baba" -> "(a)baba"
    UNIT_ASSERT(text == "ababa");
}

Y_UNIT_TEST(GlobalDisablesRepeated) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "aba",
                    "replacement": "a",
                    "global": true,
                    "repeated": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abababa";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "aba");
}

Y_UNIT_TEST(MaxRepetitions100) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "a",
                    "replacement": "b",
                    "repeated": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbaaa");
}

}  // Y_UNIT_TEST_SUITE(TReplaceByPattern_Process)


Y_UNIT_TEST_SUITE(TReplaceByPattern_Process_InplaceEqual) {

Y_UNIT_TEST(EmptyText) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "");
}

Y_UNIT_TEST(NoMatchFirst) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "xyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "xyz");
}

Y_UNIT_TEST(NoMatchGlobal) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "xyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "xyz");
}

Y_UNIT_TEST(FullMatch1First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABC");
}

Y_UNIT_TEST(FullMatch1Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABC");
}

Y_UNIT_TEST(FullMatch2First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABCabc");
}

Y_UNIT_TEST(FullMatch2Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABCABC");
}

Y_UNIT_TEST(LeadingMatch1First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABCxyz");
}

Y_UNIT_TEST(LeadingMatch1Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABCxyz");
}

Y_UNIT_TEST(LeadingMatch2First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABCabcxyz");
}

Y_UNIT_TEST(LeadingMatch2Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABCABCxyz");
}

Y_UNIT_TEST(TrailingMatch1First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABC");
}

Y_UNIT_TEST(TrailingMatch1Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABC");
}

Y_UNIT_TEST(TrailingMatch2First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABCabc");
}

Y_UNIT_TEST(TrailingMatch2Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABCABC");
}

Y_UNIT_TEST(InnerMatch1First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABCxyz");
}

Y_UNIT_TEST(InnerMatch1Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABCxyz");
}

Y_UNIT_TEST(InnerMatch2First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABCabcxyz");
}

Y_UNIT_TEST(InnerMatch2Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABCABCxyz");
}

Y_UNIT_TEST(ComplexMatchAFirst) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abbcdabcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "abbcd(abc)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc" -> "abbcd(ABC)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc"
    UNIT_ASSERT(text == "abbcdABCabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc");
}

Y_UNIT_TEST(ComplexMatchAGlobal) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abbcdabcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "abbcd(abc)(abc)klmn(abc)o(abc)pqbc(abc)(abc)(abc)uv(abc)xyzzz(abc)c" -> "abbcd(ABC)(ABC)klmn(ABC)o(ABC)pqbc(ABC)(ABC)(ABC)uv(ABC)xyzzz(ABC)c"
    UNIT_ASSERT(text == "abbcdABCABCklmnABCoABCpqbcABCABCABCuvABCxyzzzABCc");
}

Y_UNIT_TEST(ComplexMatchBFirst) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "(abc)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc" -> "(ABC)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc"
    UNIT_ASSERT(text == "ABCabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc");
}

Y_UNIT_TEST(ComplexMatchBGlobal) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "(abc)(abc)klmn(abc)o(abc)pqbc(abc)(abc)(abc)uv(abc)xyzzz(abc)c" -> "(ABC)(ABC)klmn(ABC)o(ABC)pqbc(ABC)(ABC)(ABC)uv(ABC)xyzzz(ABC)c"
    UNIT_ASSERT(text == "ABCABCklmnABCoABCpqbcABCABCABCuvABCxyzzzABCc");
}

Y_UNIT_TEST(ComplexMatchCFirst) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abbcdabcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "abbcd(abc)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc" -> "abbcd(ABC)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc"
    UNIT_ASSERT(text == "abbcdABCabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc");
}

Y_UNIT_TEST(ComplexMatchCGlobal) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABC",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abbcdabcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "abbcd(abc)(abc)klmn(abc)o(abc)pqbc(abc)(abc)(abc)uv(abc)xyzzz(abc)" -> "abbcd(ABC)(ABC)klmn(ABC)o(ABC)pqbc(ABC)(ABC)(ABC)uv(ABC)xyzzz(ABC)"
    UNIT_ASSERT(text == "abbcdABCABCklmnABCoABCpqbcABCABCABCuvABCxyzzzABC");
}

}  // Y_UNIT_TEST_SUITE(TReplaceByPattern_Process_InplaceEqual)


Y_UNIT_TEST_SUITE(TReplaceByPattern_Process_InplaceLess) {

Y_UNIT_TEST(EmptyText) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "");
}

Y_UNIT_TEST(NoMatchFirst) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "xyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "xyz");
}

Y_UNIT_TEST(NoMatchGlobal) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "xyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "xyz");
}

Y_UNIT_TEST(FullMatch1First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "AB");
}

Y_UNIT_TEST(FullMatch1Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "AB");
}

Y_UNIT_TEST(FullMatch2First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABabc");
}

Y_UNIT_TEST(FullMatch2Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABAB");
}

Y_UNIT_TEST(LeadingMatch1First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABxyz");
}

Y_UNIT_TEST(LeadingMatch1Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABxyz");
}

Y_UNIT_TEST(LeadingMatch2First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABabcxyz");
}

Y_UNIT_TEST(LeadingMatch2Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABABxyz");
}

Y_UNIT_TEST(TrailingMatch1First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrAB");
}

Y_UNIT_TEST(TrailingMatch1Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrAB");
}

Y_UNIT_TEST(TrailingMatch2First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABabc");
}

Y_UNIT_TEST(TrailingMatch2Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABAB");
}

Y_UNIT_TEST(InnerMatch1First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABxyz");
}

Y_UNIT_TEST(InnerMatch1Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABxyz");
}

Y_UNIT_TEST(InnerMatch2First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABabcxyz");
}

Y_UNIT_TEST(InnerMatch2Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABABxyz");
}

Y_UNIT_TEST(ComplexMatchAFirst) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abbcdabcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "abbcd(abc)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc" -> "abbcd(AB)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc"
    UNIT_ASSERT(text == "abbcdABabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc");
}

Y_UNIT_TEST(ComplexMatchAGlobal) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abbcdabcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "abbcd(abc)(abc)klmn(abc)o(abc)pqbc(abc)(abc)(abc)uv(abc)xyzzz(abc)c" -> "abbcd(AB)(AB)klmn(AB)o(AB)pqbc(AB)(AB)(AB)uv(AB)xyzzz(AB)c"
    UNIT_ASSERT(text == "abbcdABABklmnABoABpqbcABABABuvABxyzzzABc");
}

Y_UNIT_TEST(ComplexMatchBFirst) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "(abc)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc" -> "(AB)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc"
    UNIT_ASSERT(text == "ABabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc");
}

Y_UNIT_TEST(ComplexMatchBGlobal) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "(abc)(abc)klmn(abc)o(abc)pqbc(abc)(abc)(abc)uv(abc)xyzzz(abc)c" -> "(AB)(AB)klmn(AB)o(AB)pqbc(AB)(AB)(AB)uv(AB)xyzzz(AB)c"
    UNIT_ASSERT(text == "ABABklmnABoABpqbcABABABuvABxyzzzABc");
}

Y_UNIT_TEST(ComplexMatchCFirst) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abbcdabcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "abbcd(abc)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc" -> "abbcd(AB)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc"
    UNIT_ASSERT(text == "abbcdABabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc");
}

Y_UNIT_TEST(ComplexMatchCGlobal) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "AB",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abbcdabcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "abbcd(abc)(abc)klmn(abc)o(abc)pqbc(abc)(abc)(abc)uv(abc)xyzzz(abc)" -> "abbcd(AB)(AB)klmn(AB)o(AB)pqbc(AB)(AB)(AB)uv(AB)xyzzz(AB)"
    UNIT_ASSERT(text == "abbcdABABklmnABoABpqbcABABABuvABxyzzzAB");
}

}  // Y_UNIT_TEST_SUITE(TReplaceByPattern_Process_InplaceLess)


Y_UNIT_TEST_SUITE(TReplaceByPattern_Process_InplaceRemove) {

Y_UNIT_TEST(EmptyText) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "");
}

Y_UNIT_TEST(NoMatchFirst) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "xyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "xyz");
}

Y_UNIT_TEST(NoMatchGlobal) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "xyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "xyz");
}

Y_UNIT_TEST(FullMatch1First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "");
}

Y_UNIT_TEST(FullMatch1Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "");
}

Y_UNIT_TEST(FullMatch2First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "abc");
}

Y_UNIT_TEST(FullMatch2Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "");
}

Y_UNIT_TEST(LeadingMatch1First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "xyz");
}

Y_UNIT_TEST(LeadingMatch1Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "xyz");
}

Y_UNIT_TEST(LeadingMatch2First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "abcxyz");
}

Y_UNIT_TEST(LeadingMatch2Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "xyz");
}

Y_UNIT_TEST(TrailingMatch1First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqr");
}

Y_UNIT_TEST(TrailingMatch1Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqr");
}

Y_UNIT_TEST(TrailingMatch2First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrabc");
}

Y_UNIT_TEST(TrailingMatch2Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqr");
}

Y_UNIT_TEST(InnerMatch1First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrxyz");
}

Y_UNIT_TEST(InnerMatch1Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrxyz");
}

Y_UNIT_TEST(InnerMatch2First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrabcxyz");
}

Y_UNIT_TEST(InnerMatch2Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrxyz");
}

Y_UNIT_TEST(ComplexMatchAFirst) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abbcdabcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "abbcd(abc)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc" -> "abbcd()abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc"
    UNIT_ASSERT(text == "abbcdabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc");
}

Y_UNIT_TEST(ComplexMatchAGlobal) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abbcdabcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "abbcd(abc)(abc)klmn(abc)o(abc)pqbc(abc)(abc)(abc)uv(abc)xyzzz(abc)c" -> "abbcd()()klmn()o()pqbc()()()uv()xyzzz()c"
    UNIT_ASSERT(text == "abbcdklmnopqbcuvxyzzzc");
}

Y_UNIT_TEST(ComplexMatchBFirst) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "(abc)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc" -> "()abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc"
    UNIT_ASSERT(text == "abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc");
}

Y_UNIT_TEST(ComplexMatchBGlobal) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "(abc)(abc)klmn(abc)o(abc)pqbc(abc)(abc)(abc)uv(abc)xyzzz(abc)c" -> "()()klmn()o()pqbc()()()uv()xyzzz()c"
    UNIT_ASSERT(text == "klmnopqbcuvxyzzzc");
}

Y_UNIT_TEST(ComplexMatchCFirst) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abbcdabcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "abbcd(abc)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc" -> "abbcd()abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc"
    UNIT_ASSERT(text == "abbcdabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc");
}

Y_UNIT_TEST(ComplexMatchCGlobal) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abbcdabcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "abbcd(abc)(abc)klmn(abc)o(abc)pqbc(abc)(abc)(abc)uv(abc)xyzzz(abc)" -> "abbcd()()klmn()o()pqbc()()()uv()xyzzz()"
    UNIT_ASSERT(text == "abbcdklmnopqbcuvxyzzz");
}

}  // Y_UNIT_TEST_SUITE(TReplaceByPattern_Process_InplaceRemove)


Y_UNIT_TEST_SUITE(TReplaceByPattern_Process_NonInplace) {

Y_UNIT_TEST(EmptyText) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "");
}

Y_UNIT_TEST(NoMatchFirst) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "xyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "xyz");
}

Y_UNIT_TEST(NoMatchGlobal) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "xyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "xyz");
}

Y_UNIT_TEST(FullMatch1First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABCD");
}

Y_UNIT_TEST(FullMatch1Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABCD");
}

Y_UNIT_TEST(FullMatch2First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABCDabc");
}

Y_UNIT_TEST(FullMatch2Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABCDABCD");
}

Y_UNIT_TEST(LeadingMatch1First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABCDxyz");
}

Y_UNIT_TEST(LeadingMatch1Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABCDxyz");
}

Y_UNIT_TEST(LeadingMatch2First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABCDabcxyz");
}

Y_UNIT_TEST(LeadingMatch2Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "ABCDABCDxyz");
}

Y_UNIT_TEST(TrailingMatch1First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABCD");
}

Y_UNIT_TEST(TrailingMatch1Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABCD");
}

Y_UNIT_TEST(TrailingMatch2First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABCDabc");
}

Y_UNIT_TEST(TrailingMatch2Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABCDABCD");
}

Y_UNIT_TEST(InnerMatch1First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABCDxyz");
}

Y_UNIT_TEST(InnerMatch1Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABCDxyz");
}

Y_UNIT_TEST(InnerMatch2First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABCDabcxyz");
}

Y_UNIT_TEST(InnerMatch2Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqrabcabcxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrABCDABCDxyz");
}

Y_UNIT_TEST(ComplexMatchAFirst) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abbcdabcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "abbcd(abc)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc" -> "abbcd(ABCD)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc"
    UNIT_ASSERT(text == "abbcdABCDabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc");
}

Y_UNIT_TEST(ComplexMatchAGlobal) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abbcdabcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "abbcd(abc)(abc)klmn(abc)o(abc)pqbc(abc)(abc)(abc)uv(abc)xyzzz(abc)c" -> "abbcd(ABCD)(ABCD)klmn(ABCD)o(ABCD)pqbc(ABCD)(ABCD)(ABCD)uv(ABCD)xyzzz(ABCD)c"
    UNIT_ASSERT(text == "abbcdABCDABCDklmnABCDoABCDpqbcABCDABCDABCDuvABCDxyzzzABCDc");
}

Y_UNIT_TEST(ComplexMatchBFirst) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "(abc)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc" -> "(ABCD)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc"
    UNIT_ASSERT(text == "ABCDabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc");
}

Y_UNIT_TEST(ComplexMatchBGlobal) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabcc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "(abc)(abc)klmn(abc)o(abc)pqbc(abc)(abc)(abc)uv(abc)xyzzz(abc)c" -> "(ABCD)(ABCD)klmn(ABCD)o(ABCD)pqbc(ABCD)(ABCD)(ABCD)uv(ABCD)xyzzz(ABCD)c"
    UNIT_ASSERT(text == "ABCDABCDklmnABCDoABCDpqbcABCDABCDABCDuvABCDxyzzzABCDc");
}

Y_UNIT_TEST(ComplexMatchCFirst) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abbcdabcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "abbcd(abc)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc" -> "abbcd(ABCD)abcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc"
    UNIT_ASSERT(text == "abbcdABCDabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc");
}

Y_UNIT_TEST(ComplexMatchCGlobal) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "pattern": "abc",
                    "replacement": "ABCD",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abbcdabcabcklmnabcoabcpqbcabcabcabcuvabcxyzzzabc";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "abbcd(abc)(abc)klmn(abc)o(abc)pqbc(abc)(abc)(abc)uv(abc)xyzzz(abc)" -> "abbcd(ABCD)(ABCD)klmn(ABCD)o(ABCD)pqbc(ABCD)(ABCD)(ABCD)uv(ABCD)xyzzz(ABCD)"
    UNIT_ASSERT(text == "abbcdABCDABCDklmnABCDoABCDpqbcABCDABCDABCDuvABCDxyzzzABCD");
}

}  // Y_UNIT_TEST_SUITE(TReplaceByPattern_Process_NonInplace)


Y_UNIT_TEST_SUITE(TReplaceByRegex_Process) {

Y_UNIT_TEST(Replace) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "regex": "a",
                    "replacement": "A"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqraxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrAxyz");
}

Y_UNIT_TEST(Remove) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "regex": "a"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "pqraxyz";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "pqrxyz");
}

Y_UNIT_TEST(EmptyText) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "regex": "a",
                    "replacement": "A"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(!isModified);
    UNIT_ASSERT(text == "");
}

Y_UNIT_TEST(Global) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "regex": "aba",
                    "replacement": "a",
                    "global": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abababa";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "(aba)b(aba)" -> "(a)b(a)"
    UNIT_ASSERT(text == "aba");
}

Y_UNIT_TEST(Repeated) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "regex": "aba",
                    "replacement": "a",
                    "repeated": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abababa";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "(aba)baba" -> "(a)baba"
    // "(aba)ba"   -> "(a)ba"
    // "(aba)"     -> "(a)"
    UNIT_ASSERT(text == "a");
}

Y_UNIT_TEST(First) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "regex": "aba",
                    "replacement": "a"
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abababa";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    // "(aba)baba" -> "(a)baba"
    UNIT_ASSERT(text == "ababa");
}

Y_UNIT_TEST(GlobalDisablesRepeated) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "regex": "aba",
                    "replacement": "a",
                    "global": true,
                    "repeated": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "abababa";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "aba");
}

Y_UNIT_TEST(MaxRepetitions100) {
    const TString config = R"ELEPHANT({
            "my_batch": [
                {
                    "regex": "a",
                    "replacement": "b",
                    "repeated": true
                }
            ]
        })ELEPHANT";
    THolder<TBatchReplacer> batchReplacer;
    UNIT_ASSERT_NO_EXCEPTION(batchReplacer.Reset(new TBatchReplacer(config)));
    bool isModified;
    std::string text = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    UNIT_ASSERT_NO_EXCEPTION([&](){ isModified = batchReplacer->Process(&text, "my_batch"); }());
    UNIT_ASSERT(isModified);
    UNIT_ASSERT(text == "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbaaa");
}

}  // Y_UNIT_TEST_SUITE(TReplaceByRegex_Process)
