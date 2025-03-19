#include "cascade_item.h"
#include "remorph_item.h"
#include "tokenlogic_item.h"
#include "char_item.h"
#include "version.h"

#include <kernel/remorph/common/magic_input.h>
#include <kernel/remorph/common/verbose.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/memory/tempbuf.h>
#include <util/folder/dirut.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/ysaveload.h>

namespace NCascade {

const static TString REMORPH_PREFIX = "remorph:";
const static TString TOKENLOGIC_PREFIX = "tokenlogic:";
const static TString CHAR_PREFIX = "rem-char:";

inline TString ResolveAbsolutePath(const TString& baseDir, const TString& path) {
    if (!path.empty()) {
        TTempBuf buf;
        ResolvePath(path.data(), baseDir.data(), buf.Data(), false);
        return buf.Data();
    }
    return path;
}

void TCascadeBase::InitSubCascades(const NGzt::TGazetteer& gzt) {
    for (TCascadeItems::iterator iCascade = SubCascades.begin(); iCascade != SubCascades.end(); ++iCascade) {
        TArticlePtr article;
        if (!gzt.ArticlePool().FindCustomArticleByName(iCascade->Get()->ArticleTitle, article))
            throw yexception() << "Cannot load custom Gazetteer article '" << iCascade->Get()->ArticleTitle << "'";

        iCascade->Get()->Init(article, gzt);
        GetMatcher().GetUsedGztItem().Add(iCascade->Get()->GetMatcher().GetUsedGztItem());
        CascadeArticles.Add(article);
    }
}

template <class TCascadeType>
bool TCascadeBase::InitSubCascadeType(const NGzt::TGazetteer& gzt, const TString& baseDir,
    const NGzt::TArticlePtr& article, const TString& prefix) {
    bool hasKey = false;
    for (NGzt::TCustomKeyIterator it(article, prefix); it.Ok(); ++it) {
        TString realPath = ResolveAbsolutePath(baseDir, *it);
        TCascadeItemPtr inner;
        try {
            inner = TCascadeType::LoadItemFromFile(gzt, baseDir, article, realPath);
        } catch (const yexception& e) {
            throw yexception() << "Error loading cascaded " << prefix << realPath.Quote() << ": " << e.what();
        }
        GetMatcher().GetUsedGztItem().Add(inner->GetMatcher().GetUsedGztItem());
        SubCascades.push_back(inner);
        hasKey = true;
    }
    return hasKey;
}

void TCascadeBase::InitSubCascadesFromArticle(const NGzt::TGazetteer& gzt, const TString& baseDir,
    const NGzt::TArticlePtr& article) {

    REPORT(INFO, "Processing custom article '" << article.GetTitle() << "'");
    CascadeArticles.Add(article);
    bool hasKnownKey = false;
    hasKnownKey = InitSubCascadeType<TRemorphCascadeItem>(gzt, baseDir, article, REMORPH_PREFIX) || hasKnownKey;
    hasKnownKey = InitSubCascadeType<TTokenLogicCascadeItem>(gzt, baseDir, article, TOKENLOGIC_PREFIX) || hasKnownKey;
    hasKnownKey = InitSubCascadeType<TCharCascadeItem>(gzt, baseDir, article, CHAR_PREFIX) || hasKnownKey;

    if (!hasKnownKey) {
        throw yexception() << "Custom article '" << article.GetTitle() << "' has none of supported custom keys";
    }
}

struct TCascadeOrder {
    bool operator() (const TCascadeItemPtr& l, const TCascadeItemPtr& r) const {
        // Order by priority descending (cascade with higher priority will be executed first)
        // Cascades with equal priority are ordered by article ID
        return l->Type < r->Type
            || (l->Type == r->Type && l->Priority > r->Priority)
            || (l->Type == r->Type && l->Priority == r->Priority && l->Article.GetId() < r->Article.GetId());
    }
};

void TCascadeBase::ScanCascades(const NGzt::TGazetteer& gzt, const TString& baseDir) {
    THashSet<TUtf16String> gztItems;
    GetMatcher().CollectUsedGztItems(gztItems);
    for (THashSet<TUtf16String>::const_iterator i = gztItems.begin(); i != gztItems.end(); ++i) {
        NGzt::TArticlePtr article;
        if (gzt.ArticlePool().FindCustomArticleByName(*i, article)
            && !CascadeArticles.Has(article)) { // Skip already processed articles

            InitSubCascadesFromArticle(gzt, baseDir, article);
        }
    }
    ::StableSort(SubCascades.begin(), SubCascades.end(), TCascadeOrder());
}

void TCascadeBase::LoadRootCascadeFromFile(const TString& filePath, const TGazetteer* gzt, const TString& baseDir) {
    {
        TIFStream fi(filePath);
        TMagicInput in(fi);
        if (in.HasMagic()) {
            try {
                if (in.HasType(CASCADE_TYPE)) {
                    REPORT(INFO, "Loading cascade from '" << filePath << "'");
                    in.CheckVersion(CASCADE_BINARY_VERSION);
                    if (nullptr == gzt) {
                        throw yexception() << "Loaded rules require Gazetteer instance to be specified";
                    }
                    ReadCascadeData(in, false);
                    GetMatcher().Init(gzt);
                    InitSubCascades(*gzt);
                } else {
                    // Load pure matcher without cascades
                    in.Reset();
                    GetMatcher().LoadFromStream(in, true);
                    GetMatcher().Init(gzt);
                }
            } catch (TBinaryError& error) {
                error.SetFilePath(filePath);
                throw error;
            }
            return;
        }
    }
    GetMatcher().LoadFromFile(filePath, gzt);
    if (!GetMatcher().GetUsedGztItem().Empty()) {
        if (nullptr == gzt) {
            throw yexception() << "Loaded rules require Gazetteer instance to be specified";
        }
        REPORT(INFO, "Building cascades...");
        ScanCascades(*gzt, baseDir);
    }
}

void TCascadeBase::LoadRootCascadeFromFile(const TString& filePath, const TGazetteer* gzt) {
    LoadRootCascadeFromFile(filePath, gzt, TFsPath(filePath).Parent().RealPath());
}

void TCascadeBase::LoadRootCascadeFromStream(IInputStream& in, const TGazetteer* gzt) {
    TMagicInput magicIn(in);
    magicIn.CheckMagic();
    if (magicIn.HasType(CASCADE_TYPE)) {
        magicIn.CheckVersion(CASCADE_BINARY_VERSION);
        if (nullptr == gzt) {
            throw yexception() << "Loaded rules require Gazetteer instance to be specified";
        }
        ReadCascadeData(magicIn, false);
        GetMatcher().Init(gzt);
        InitSubCascades(*gzt);
    } else {
        // Load pure matcher without cascades
        magicIn.Reset();
        GetMatcher().LoadFromStream(magicIn, true);
        GetMatcher().Init(gzt);
    }
}

void TCascadeBase::ReadCascadeData(IInputStream& in, bool signature) {
    if (signature) {
        TMagicCheck(in).Check(CASCADE_TYPE, CASCADE_BINARY_VERSION);
    }
    GetMatcher().LoadFromStream(in, true);
    ::Load(&in, SubCascades);
}

void TCascadeBase::LoadSubCascadeFromFile(const TString& filePath, const TGazetteer& gzt, const TString& baseDir) {
    {
        TIFStream fi(filePath);
        TMagicInput in(fi);
        if (in.HasMagic() && in.HasType(CASCADE_TYPE)) {
            in.CheckVersion(CASCADE_BINARY_VERSION);
            REPORT(INFO, "Loading binary cascade from '" << filePath << "'");
            ReadCascadeData(in, false);
            GetMatcher().Init(&gzt);
            InitSubCascades(gzt);
            return;
        }
    }
    GetMatcher().LoadFromFile(filePath, &gzt);
    ScanCascades(gzt, baseDir);
}

void TCascadeBase::SavePrefix(IOutputStream& out) const {
    ::WriteMagic(out, CASCADE_TYPE, CASCADE_BINARY_VERSION);
}

void TCascadeBase::SaveData(IOutputStream& out) const {
    ::Save(&out, SubCascades);
}

} // NCascade
