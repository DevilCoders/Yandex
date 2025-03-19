#pragma once

namespace NTest {

    enum class EMode {
        Apply,
        ApplyAll,
    };

    struct TOptions {
        void Parse(int argc, const char* argv[]);

    public:
        EMode Mode = EMode::Apply;
    };

} // namespace NTest
