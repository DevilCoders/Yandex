#include <library/cpp/charset/recyr.hh>
#include <util/string/vector.h>
#include <util/string/split.h>
#include <library/cpp/testing/unittest/registar.h>

#include <kernel/segnumerator/html_markers.h>

/*
 * !!!!!IMPORTANT!!!!!
 * This code uses an ugly hack making it encoding-dependent.
 * Use only cp1251 for it.
 */
class TMarkersTest: public TTestBase {
UNIT_TEST_SUITE(TMarkersTest);
        UNIT_TEST(FooterCSSTest);
        UNIT_TEST(FooterTextTest);

        UNIT_TEST(AdsCSSTest);
        UNIT_TEST(AdsTextTest);

        UNIT_TEST(CommentsCSSTest);
        UNIT_TEST(CommentsTextTest);

        UNIT_TEST(HeaderCSSTest);
        //        UNIT_TEST(MenuCSSTest);
        UNIT_TEST(PollCSSTest);
        UNIT_TEST_SUITE_END();
private:
    typedef TVector<TString>::const_iterator TIt;
    typedef bool (*TFunc)(const char *, ui32);
    typedef bool (*TFunc32)(const wchar16*, ui32);

    template<TFunc FUNC>
    void Check(bool match, const char* src) {
        TVector<TString> vs = SplitString(src, "|", KEEP_EMPTY_TOKENS);
        for (TIt it = vs.begin(); it != vs.end(); ++it) {
            bool res = FUNC(it->c_str(), it->size());
            UNIT_ASSERT_VALUES_EQUAL_C(res, match, it->c_str());
        }
    }

    template<TFunc32 FUNC>
    void Check32(bool match, const char* src) {
        TVector<TString> vs = SplitString(src, "|", KEEP_EMPTY_TOKENS);
        for (TIt it = vs.begin(); it != vs.end(); ++it) {
            size_t read = 0;
            size_t written = 0;
            size_t len = it->size();
            TCharTemp buf(len);
            wchar16* arr = buf.Data();
            RecodeToUnicode(CODES_WIN, it->c_str(), arr, len, len, read, written);

            bool res = FUNC(arr, written);
            UNIT_ASSERT_VALUES_EQUAL_C(res, match, Recode(CODES_WIN, CODES_UTF8, *it).c_str());
        }
    }
    void AdsCSSTest() {
        using NSegm::NPrivate::CheckAdsCSSMarker;

        Check<CheckAdsCSSMarker> (true, "_adsv|adl|ad0|ad|ads|rekl|banner|spons|articlebanner");
        Check<CheckAdsCSSMarker> (false, "bad|add|respons|banned|");
    }

    void FooterCSSTest() {
        using NSegm::NPrivate::CheckFooterCSSMarker;

        Check<CheckFooterCSSMarker> (true, "footer|copyr|kopirajt|kontact");
        Check<CheckFooterCSSMarker> (false, "copy|docopy|footnote|aCopy|nofoot|");
    }

    void CommentsCSSTest() {
        using NSegm::NPrivate::CheckCommentsCSSMarker;

        Check<CheckCommentsCSSMarker> (true, "comment|discuss|reply");
    }

    void HeaderCSSTest() {
        using NSegm::NPrivate::CheckHeaderCSSMarker;

        Check<CheckHeaderCSSMarker> (true,
                "strong|hh6|b|textHead|tit|caption|capit|titl|bold|headcolumn|spiegelhead");
        Check<CheckHeaderCSSMarker> (false, "hr|rowB|ch6|bl|");
    }

    void MenuCSSTest() {
         using NSegm::NPrivate::CheckMenuCSSMarker;

//         Check<CheckMenuCSSMarker> (true, "bread|path|nav|menu|pagin|listal|link|map|menu|calend|caltab");
     }

    void PollCSSTest() {
        using NSegm::NPrivate::CheckPollCSSMarker;

        Check<CheckPollCSSMarker> (true, "opros|golosovalka|surveying|askMnenie|doVote|div0poll");
    }

    void FooterTextTest() {
        using NSegm::NPrivate::CheckFooterTextMarker;

        Check32<CheckFooterTextMarker> (true,
                "� 2001�2009 �������|2007 �|�|� foobar 2007|CoPyRiGhT 2007"
                    "|(�) ��������� ������ ����� 2007"
                    "|��� ������� ��������. �������� ����������� ����������, "
                    "����� ��������� �������������� ��������� 2007"
                    "|�Copyright. 2002 �All rights reserved.");
        Check32<CheckFooterTextMarker> (false, "foobar 2007|copy right, dude|copyright|copyright fags");
    }

    void CommentsTextTest() {
        using NSegm::NPrivate::CheckCommentsTextMarker;

        Check32<CheckCommentsTextMarker> (true, "Comments|������|�����������|������|����������");
        Check32<CheckCommentsTextMarker> (false, "|");
    }

    void AdsTextTest() {
        using NSegm::NPrivate::CheckAdsTextMarker;

        Check32<CheckAdsTextMarker> (true, "Ads|Advertisments|Advertising|Banners"
            "|�����������|���������|������");
        Check32<CheckAdsTextMarker> (false, "bad|add|");
    }

};
UNIT_TEST_SUITE_REGISTRATION(TMarkersTest)
;
