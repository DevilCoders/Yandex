#include "fio.h"

#include <kernel/qtree/richrequest/richnode.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NSnippets {
    static constexpr TWtringBuf fioname{u"fioname"};
    static constexpr TWtringBuf fiinoinname{u"fiinoinname"};
    static constexpr TWtringBuf finame{u"finame"};
    static constexpr TWtringBuf fiinname{u"fiinname"};

    static inline bool IsFioname(const TUtf16String& s) {
        return  s == fioname;
    }

    static inline bool IsFiinoinname(const TUtf16String& s) {
        return  s == fiinoinname;
    }

    static inline bool IsFiname(const TUtf16String& s) {
        return  s == finame;
    }

    static inline bool IsFiinname(const TUtf16String& s) {
        return  s == fiinname;
    }

    static inline bool IsFioZone(const TUtf16String& s) {
        return IsFioname(s) || IsFiinoinname(s) || IsFiname(s) || IsFiinname(s);
    }

    bool HasFio(const TRichRequestNode& node) {
        typedef TNodeSequence::const_iterator TChildIterator;

        for (TChildIterator it = node.MiscOps.begin(); it != node.MiscOps.end(); ++it) {
            if (IsZone(**it) && IsFioZone((*it)->GetTextName())) {
                return true;
            }
        }
        for (TChildIterator chi = node.Children.begin(); chi != node.Children.end(); ++chi) {
            if (HasFio(*chi->Get())) {
                return true;
            }
        }
        TChildIterator msc;
        for (msc = node.MiscOps.begin(); msc != node.MiscOps.end(); msc++) {
            if (oRefine == (*msc)->Op() || oRestrDoc == (*msc)->Op()) {
                if (HasFio(*msc->Get())) {
                    return true;
                }
            }
        }

        for (NSearchQuery::TForwardMarkupIterator<TSynonym, true> j(node.Markup()); !j.AtEnd(); ++j) {
            if (HasFio(*j.GetData().SubTree)) {
                return true;
            }
        }
        return false;
    }
}
