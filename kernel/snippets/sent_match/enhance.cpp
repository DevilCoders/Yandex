#include "enhance.h"
#include <kernel/snippets/sent_match/glue.h>
#include "extra_attrs.h"

#include <kernel/snippets/config/config.h>

#include <kernel/snippets/strhl/hlmarks.h>

namespace NSnippets {

void EnhanceSnippet(const TEnhanceSnippetConfig& cfg, TVector<TZonedString>& snipVec, TExtraSnipAttrs& extraSnipAttrs)
{
    for (size_t i = 0; i < snipVec.size(); ++i) {
        if (cfg.Config.NeedDecapital()) {
            snipVec[i] = TGluer::Decapitalize(snipVec[i], cfg.DocLangId, cfg.OutputHandler);
        }
        if (!cfg.IsByLink) { // SNIPPETS-249
            snipVec[i] = TGluer::CutTrash(snipVec[i]);
        }
        if (cfg.Config.IsMarkParagraphs()) {
            snipVec[i].Zones[+TZonedString::ZONE_PARABEG].Mark = &TGluer::ParaMark;
        }
        if (cfg.Config.UseTableSnip()) {
            snipVec[i].Zones[+TZonedString::ZONE_TABLE_CELL].Mark = &TABLECELL_MARK;
            snipVec[i] = TGluer::EmbedTableCellMarks(snipVec[i]);
        }
        if (cfg.Config.LinksInSnip()) {
            snipVec[i].Zones[+TZonedString::ZONE_LINK].Mark = &LINK_MARK;
            TVector<TString> links;
            snipVec[i] = TGluer::EmbedLinkMarks(snipVec[i], links, cfg.Url);
            for (size_t j = 0; j < links.size(); ++j) {
                extraSnipAttrs.AppendLinkAttr(links[j]);
            }
        }
        snipVec[i] = TGluer::EmbedPara(snipVec[i]);
        snipVec[i].Zones[+TZonedString::ZONE_MATCHED_PHONE].Mark = &DEFAULT_MARKS;
        if (cfg.Config.PaintAllPhones()) {
            snipVec[i].Zones[+TZonedString::ZONE_PHONES].Mark = &DEFAULT_MARKS;
        }
        snipVec[i].Zones[+TZonedString::ZONE_EXTSNIP].Mark = &EXT_MARK;
        snipVec[i] = TGluer::EmbedExtMarks(snipVec[i]);
        FixWeirdChars(snipVec[i].String); //fine, doesn't change zones
    }
}

}
