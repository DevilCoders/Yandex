#include "robots_txt.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/i18n/i18n.h>
#include <library/cpp/charset/doccodes.h>

namespace NSnippets {

namespace {

const char* const REPL_ID = "robots_txt_all_stub";
const char* const HEADLINE_SRC = REPL_ID;
constexpr TStringBuf INFO_MSG_ID = "robots_txt_info";

} // anonymous namespace

TRobotsTxtStubReplacer::TRobotsTxtStubReplacer(bool isFakeForBan)
    : IReplacer(REPL_ID)
    , IsFakeForBan(isFakeForBan)
{
}

void TRobotsTxtStubReplacer::DoWork(TReplaceManager* manager) {
    Y_ASSERT(manager);
    if (!IsFakeForBan)
        return;

    const TReplaceContext& ctx = manager->GetContext();
    if (ctx.Cfg.RobotsTxtSkipNPS() && ctx.IsByLink && !ctx.Snip.Snips.empty())
        return;
    TReplaceResult res;
    res.UseText(Localize(INFO_MSG_ID, ctx.Cfg.GetUILang()), HEADLINE_SRC);
    res.UseTitle(ctx.SuperNaturalTitle);
    manager->Commit(res, MRK_ROBOTS_TXT);
}

} // namespace NSnippets
