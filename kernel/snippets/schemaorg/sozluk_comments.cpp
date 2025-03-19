#include "sozluk_comments.h"

#include <util/charset/wide.h>
#include <util/string/strip.h>
#include <util/string/subst.h>

namespace {
    const TUtf16String BKZ = u"(bkz";
    const TUtf16String ARA = u"(ara";
    const TUtf16String SPOILER = u"--- spoiler ---";
    const TUtf16String ASTERISK = u"*";
    const wchar16 LEFT_TRASH[] = {' ', '-', '+', '*', 0xB7 /* Middle Dot */, 0x2022 /* Bullet */, 0};

    void CutLeftTrash(TUtf16String& str) {
        size_t pos = str.find_first_not_of(LEFT_TRASH);
        if (pos == str.npos) {
            str.clear();
        } else if (pos) {
            str.erase(0, pos);
        }
    }

    size_t FindCloseParen(const TUtf16String& str, size_t openPos) {
        int balance = 1;
        for (size_t closePos = openPos + 1; closePos < str.size(); ++closePos) {
            if (str[closePos] == '(') {
                ++balance;
            } else if (str[closePos] == ')') {
                --balance;
                if (balance == 0) {
                    return closePos;
                }
            }
        }
        return str.size();
    }

    void CutBkzParen(TUtf16String& comment) {
        for (size_t i = 0; i < comment.size();) {
            size_t openPos = comment.find(BKZ, i);
            if (openPos == comment.npos) {
                break;
            }
            size_t closePos = FindCloseParen(comment, openPos);
            if (closePos < comment.size()) {
                if (closePos + 1 < comment.size() && comment[closePos + 1] == '(') {
                    *(comment.begin() + closePos) = ' ';
                } else {
                    comment.erase(closePos, 1);
                }
                comment.erase(openPos, 1);
            }
            i = closePos;
        }
    }

    void CutAra(TUtf16String& comment) {
        for (size_t i = 0; i < comment.size();) {
            size_t openPos = comment.find(ARA, i);
            if (openPos == comment.npos) {
                break;
            }
            size_t closePos = FindCloseParen(comment, openPos);
            if (closePos < comment.size()) {
                comment.erase(openPos, closePos - openPos + 1);
            } else {
                i = closePos;
            }
        }
    }

} // anonymous namespace

namespace NSchemaOrg {
    TUtf16String TSozlukComments::CleanComment(const TUtf16String& comment) {
        TUtf16String str = comment;
        CutBkzParen(str);
        CutAra(str);
        SubstGlobal(str, ASTERISK, TUtf16String());
        SubstGlobal(str, SPOILER, TUtf16String());
        CutLeftTrash(str);
        str = StripStringRight(str);
        return str;
    }

    TVector<TUtf16String> TSozlukComments::GetCleanComments(size_t maxCount) const {
        TVector<TUtf16String> comments;
        for (const TUtf16String& comment : Comments) {
            if (comments.size() >= maxCount) {
                break;
            }
            TUtf16String cleanComment = CleanComment(comment);
            if (cleanComment) {
                comments.push_back(cleanComment);
            }
        }
        return comments;
    }

} // namespace NSchemaOrg
