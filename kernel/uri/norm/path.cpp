/*
 *  Created on: Dec 3, 2011
 *      Author: albert@
 *
 * $Id$
 */

#include "path.h"

#include <library/cpp/uri/common.h>

namespace Nydx {
namespace NUriNorm {

namespace {

struct TLessNoCase
{
    bool operator()(const TStringBuf& lt, const TStringBuf &rt) const
    {
        return 0 > NUri::CompareNoCase(lt, rt);
    }
};

}

class DefaultPathSuffix
{
    TSetT<TStringBuf, TLessNoCase> Name_;

public:
    DefaultPathSuffix()
    {
        // taken from help.godaddy.com/article/60
        Name_.Add("default");
        Name_.Add("index");
        Name_.Add("home");
        Name_.Add("welcome");
    }
    bool Has(TStringBuf key) const
    {
        const TStringBuf ext = key.RNextTok('.');
        return ext.IsInited() && Name_.Has(key);
    }
};

static const DefaultPathSuffix DefPathSuf;

/**
 * Normalizes the path part of a URI.
 * @param[in,out] path the path string
 * @return true if the path has changed
 */
void TPathNormalizer::NormalizePath()
{
    // XXX: remove trailing slash from path; although technically wrong,
    // this is the only way to ensure matching in case of minor differences
    // XXX: remove trailing default files

    // we don't touch these without looking at the scheme
    if (GetURI().IsOpaque())
        return;

    do {
        TStringBuf file = Path_.SubStr(1 + Path_.rfind('/'));

        if (!file.empty() && Flags_.GetPathRemoveIndexPage() && DefPathSuf.Has(file)) {
            // remove the trailing index.html and similar
            Path_.Chop(file.length());
            file.Clear();
            Changed_ = true;
        }

        if (Path_.empty()) {
            if (GetField(::NUri::TUri::FieldHost).empty())
                return;
            SetImpl(TStringBuf("/"));
            break;
        }

        if (file.empty()) { // path ends in a slash
            if (1 == Path_.length() || !Flags_.GetPathRemoveTrailSlash())
                return;
            Path_.Chop(1); // remove the trailing slash
            break;
        }

        if (Flags_.GetPathAppendDirTrailSlash()) {
            if (TStringBuf::npos == file.find('.')) {
                PathStr_ = Path_;
                PathStr_ += '/';
                SetImpl(PathStr_);
            }
            return;
        }

        // nothing's changed
        return;
    }
    while (false);

    Changed_ = true;
}

void TPathNormalizer::CleanupImpl(const TStringBuf &val)
{
    Path_ = val;
    NormalizePath();
    Parsed_ = true;

    if (!Flags_.GetLowercaseURL() && Flags_.GetLowercasePath()) {
        for (size_t idx = 0; idx != Path_.length(); ++idx)
        {
            if (isupper(Path_[idx])) {
                if (Path_.data() != PathStr_.data())
                    PathStr_.AssignNoAlias(Path_);
                TString::iterator cur = PathStr_.begin() + idx;
                do
                    *cur = ::tolower(*cur);
                while (PathStr_.end() != ++cur);
                SetImpl(PathStr_);
                break;
            }
        }
    }
}

}
}
