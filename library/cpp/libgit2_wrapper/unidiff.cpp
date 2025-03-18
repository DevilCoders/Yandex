#include "unidiff.h"

#include <contrib/libs/libgit2/include/git2/diff.h>

#include <library/cpp/colorizer/colors.h>

#include <util/generic/yexception.h>
#include <util/stream/buffered.h>

namespace {
    struct TDiffContext {
        IOutputStream& Out;
        NColorizer::TColors Colors;
    };

    int BinaryCallback(const git_diff_delta*,
                       const git_diff_binary*,
                       void* context) {
        auto ctx = static_cast<TDiffContext*>(context);
        ctx->Out << "Binary files differ" << Endl;
        return 0;
    };

    int HunkCallback(const git_diff_delta*,
                     const git_diff_hunk* hunk,
                     void* context) {
        auto ctx = static_cast<TDiffContext*>(context);
        ctx->Out << ctx->Colors.CyanColor();
        ctx->Out << TStringBuf(hunk->header, hunk->header_len);
        ctx->Out << ctx->Colors.OldColor();
        return 0;
    };

    int LineCallback(const git_diff_delta*,
                     const git_diff_hunk*,
                     const git_diff_line* line,
                     void* context) {
        auto ctx = static_cast<TDiffContext*>(context);
        bool colored = false;
        bool prefix = true;
        switch (line->origin) {
            case '+':
                ctx->Out << ctx->Colors.GreenColor();
                colored = true;
                break;
            case '-':
                ctx->Out << ctx->Colors.RedColor();
                colored = true;
                break;
            case '=':
            case '>':
            case '<':
                prefix = false;
        }
        TStringBuf data(line->content, line->content_len);
        if (prefix) {
            ctx->Out << line->origin;
        }
        ctx->Out << data;
        if (colored) {
            ctx->Out << ctx->Colors.OldColor();
        }
        return 0;
    };

} // namespace

namespace NLibgit2 {

    void UnifiedDiff(const TStringBuf l,
                     const TStringBuf r,
                     const ui32 context,
                     IOutputStream& out,
                     bool colored,
                     bool ignoreWhitespace,
                     bool ignoreWhitespaceChange,
                     bool ignoreWhitespaceEOL) {

        TAdaptiveBufferedOutput cout(&out);
        TDiffContext ctx = {cout, NColorizer::TColors(colored)};

        git_diff_options options;
        Y_ENSURE(git_diff_init_options(&options, GIT_DIFF_OPTIONS_VERSION) == 0);
        options.context_lines = context;

        if (ignoreWhitespace) {
            options.flags |= GIT_DIFF_IGNORE_WHITESPACE;
        }
        if (ignoreWhitespaceChange) {
            options.flags |= GIT_DIFF_IGNORE_WHITESPACE_CHANGE;
        }
        if (ignoreWhitespaceEOL) {
            options.flags |= GIT_DIFF_IGNORE_WHITESPACE_EOL;
        }

        auto res = git_diff_buffers(
            l.Data(), l.Size(), nullptr,
            r.Data(), r.Size(), nullptr,
            &options,
            nullptr,
            BinaryCallback,
            HunkCallback,
            LineCallback,
            &ctx);
        Y_ENSURE(res == 0);
    }

} // namespace NLibgit2
