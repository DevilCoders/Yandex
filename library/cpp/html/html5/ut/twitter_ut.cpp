#include <library/cpp/html/html5/parse.h>
#include <library/cpp/html/face/onchunk.h>
#include <library/cpp/html/storage/storage.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/strbuf.h>

namespace {
    const TStringBuf URL = "http://twitter.com/sofya_nv";

    //TODO: check attributes for links
    bool CheckTagSequence(const NHtml::TStorage& storage, const HT_TAG* tags, size_t tagsCount) {
        size_t index = 0;
        for (auto cur = storage.Begin(); cur != storage.End(); ++cur) {
            const THtmlChunk* const chunk = GetHtmlChunk(cur);
            if (index < tagsCount) {
                if (chunk->flags.apos != HTLEX_TEXT) {
                    UNIT_ASSERT(chunk->Tag->is(tags[index]));
                } else {
                    //!!!NOTE: HT_any means text.
                    UNIT_ASSERT(tags[index] == HT_any);
                }
                ++index;
            } else {
                return false;
            }
        }
        return true;
    }

    template <size_t Len>
    static void CheckTweet(const TStringBuf& html, const HT_TAG (&tags)[Len]) {
        NHtml::TStorage storage;
        NHtml::TParserResult processor(storage);
        NHtml5::ParseHtml(html, &processor, URL);

        UNIT_ASSERT(CheckTagSequence(storage, tags, Len));
    }

}

Y_UNIT_TEST_SUITE(TTwitterTest) {
    Y_UNIT_TEST(CheckAttrs) {
        const TStringBuf data = TStringBuf("<p class=\"js-tweet-text tweet-text\" lang=\"ru\" data-aria-label-part=\"0\">Открываю <a href=\"http://t.co/a44WljjX1x\" rel=\"nofollow\" dir=\"ltr\" data-expanded-url=\"https://weather.yahoo.com/\" class=\"twitter-timeline-link\" target=\"_blank\" title=\"https://weather.yahoo.com/\" ><span class=\"tco-ellipsis\"></span><span class=\"invisible\">https://</span><span class=\"js-display-url\">weather.yahoo.com</span><span class=\"invisible\">/</span><span class=\"tco-ellipsis\"><span class=\"invisible\">&nbsp;</span></span></a> а там фоном ну чуть ли не мой дом, экая приятная неожиданность :)</p>");

        NHtml::TStorage storage;
        NHtml::TParserResult processor(storage);
        NHtml5::ParseHtml(data, &processor, URL);

        for (auto cur = storage.Begin(); cur != storage.End(); ++cur) {
            const THtmlChunk* const chunk = GetHtmlChunk(cur);
            if (chunk->flags.apos == HTLEX_START_TAG && *(chunk->Tag) == HT_A) {
                UNIT_ASSERT(chunk->AttrCount == 6);
                for (size_t i = 0; i < chunk->AttrCount; ++i) {
                    TString name(chunk->text + chunk->Attrs[i].Name.Start, chunk->Attrs[i].Name.Leng);
                    if (name == "href") {
                        TString value(chunk->text + chunk->Attrs[i].Value.Start, chunk->Attrs[i].Value.Leng);
                        UNIT_ASSERT(value == "https://weather.yahoo.com/");
                    }
                    if (name == "rel") {
                        UNIT_ASSERT(chunk->Attrs[i].Value.Leng == 0);
                    }
                }
            }
        }
    }

    Y_UNIT_TEST(TextTwitPage) {
        const TStringBuf data = TStringBuf("<p class=\"js-tweet-text tweet-text\" lang=\"ru\" data-aria-label-part=\"0\">Привет <span>всем!</span></p>");
        const HT_TAG tagsSeq[] = {HT_HTML, HT_HEAD, HT_HEAD, HT_BODY, HT_P,
                                  HT_any, HT_SPAN, HT_any, HT_SPAN, HT_P, HT_BODY, HT_HTML};

        CheckTweet(data, tagsSeq);
    }

    Y_UNIT_TEST(BaseTwitPage) {
        const TStringBuf data = TStringBuf("<p class=\"js-tweet-text tweet-text\" lang=\"ru\" data-aria-label-part=\"0\">Открываю <a href=\"http://t.co/a44WljjX1x\" rel=\"nofollow\" dir=\"ltr\" data-expanded-url=\"https://weather.yahoo.com/\" class=\"twitter-timeline-link\" target=\"_blank\" title=\"https://weather.yahoo.com/\" ><span class=\"tco-ellipsis\"></span><span class=\"invisible\">https://</span><span class=\"js-display-url\">weather.yahoo.com</span><span class=\"invisible\">/</span><span class=\"tco-ellipsis\"><span class=\"invisible\">&nbsp;</span></span></a> а там фоном ну чуть ли не мой дом, экая приятная неожиданность :)</p>");
        const HT_TAG tagsSeq[] = {HT_HTML, HT_HEAD, HT_HEAD, HT_BODY, HT_P,
                                  HT_A, HT_any, HT_SPAN, HT_SPAN, HT_SPAN, HT_any, HT_SPAN, HT_SPAN,
                                  HT_any, HT_SPAN, HT_SPAN, HT_any, HT_SPAN, HT_SPAN, HT_SPAN, HT_any,
                                  HT_SPAN, HT_SPAN, HT_any, HT_A, HT_P, HT_BODY, HT_HTML};

        CheckTweet(data, tagsSeq);
    }

    Y_UNIT_TEST(TwitPageWith2Links) {
        const TStringBuf data = TStringBuf("<p class=\"js-tweet-text tweet-text\" lang=\"ru\" data-aria-label-part=\"0\">Идеальный <a href=\"http://t.co/a44WljjX1x\" rel=\"nofollow\" dir=\"ltr\" data-expanded-url=\"https://weather.yahoo.com/\" class=\"twitter-timeline-link\" target=\"_blank\" title=\"https://weather.yahoo.com/\" >weather.yahoo.com</a> осенний день в <a href=\"http://t.co/a44WljjX1x\" rel=\"nofollow\" dir=\"ltr\" data-expanded-url=\"https://weather.yahoo.com/\" class=\"twitter-timeline-link\" target=\"_blank\" title=\"https://weather.yahoo.com/\" >weather.yahoo.com</a> Ekb!</p>");
        const HT_TAG tagsSeq[] = {HT_HTML, HT_HEAD, HT_HEAD, HT_BODY, HT_P,
                                  HT_A, HT_any, HT_any, HT_any, HT_A, HT_A, HT_any, HT_any, HT_A,
                                  HT_P, HT_BODY, HT_HTML};

        CheckTweet(data, tagsSeq);
    }

    Y_UNIT_TEST(AnotherTwitPageWith2Links) {
        const TStringBuf data = TStringBuf("<p class=\"js-tweet-text tweet-text\" lang=\"ru\" data-aria-label-part=\"0\">Идеальный <a href=\"http://t.co/a44WljjX1x\" rel=\"nofollow\" dir=\"ltr\" data-expanded-url=\"https://weather.yahoo.com/\" class=\"twitter-timeline-link\" target=\"_blank\" title=\"https://weather.yahoo.com/\" >weather.yahoo.com</a> осенний день в <a href=\"http://t.co/a44WljjX1x\" rel=\"nofollow\" dir=\"ltr\" data-expanded-url=\"https://weather.yahoo.com/\" class=\"twitter-timeline-link\" target=\"_blank\" title=\"https://weather.yahoo.com/\" >weather.yahoo.com</a></p>");
        const HT_TAG tagsSeq[] = {HT_HTML, HT_HEAD, HT_HEAD, HT_BODY, HT_P,
                                  HT_A, HT_any, HT_any, HT_any, HT_A, HT_A, HT_any, HT_A,
                                  HT_P, HT_BODY, HT_HTML};

        CheckTweet(data, tagsSeq);
    }

    Y_UNIT_TEST(TweetWithHash) {
        const TStringBuf data = TStringBuf("<p class=\"js-tweet-text tweet-text\" lang=\"ru\" data-aria-label-part=\"0\">Идеальный осенний день в <a href=\"/hashtag/ekb?src=hash\" data-query-source=\"hashtag_click\" class=\"twitter-hashtag pretty-link js-nav\" dir=\"ltr\" ><b>#ekb</b></a> !</p>");
        const HT_TAG tagsSeq[] = {HT_HTML, HT_HEAD, HT_HEAD, HT_BODY, HT_P,
                                  HT_any, HT_A, HT_B, HT_any, HT_B, HT_A, HT_any,
                                  HT_P, HT_BODY, HT_HTML};

        CheckTweet(data, tagsSeq);
    }

    Y_UNIT_TEST(TweetWithPic) {
        const TStringBuf data = TStringBuf("<p class=\"js-tweet-text tweet-text\" lang=\"ru\" data-aria-label-part=\"0\">Даже и не знаю, пошла бы я к ним танцевать. <a href=\"http://t.co/a44WljjX1x\" rel=\"nofollow\" dir=\"ltr\" data-expanded-url=\"https://weather.yahoo.com/\" class=\"twitter-timeline-link\" target=\"_blank\" title=\"https://weather.yahoo.com/\" >weather.yahoo.com</a> Не дай бог стать такой красавицей)) <a href=\"http://t.co/Je9jU3XTqZ\" class=\"twitter-timeline-link u-hidden\" data-pre-embedded=\"true\" dir=\"ltr\" >pic.twitter.com/Je9jU3XTqZ</a> совсем </p>");
        const HT_TAG tagsSeq[] = {HT_HTML, HT_HEAD, HT_HEAD, HT_BODY, HT_P,
                                  HT_A, HT_any, HT_any, HT_any, HT_A, HT_A, HT_any, HT_any, HT_A,
                                  HT_P, HT_BODY, HT_HTML};

        CheckTweet(data, tagsSeq);
    }

    Y_UNIT_TEST(LinkHashTweet) {
        const TStringBuf data = TStringBuf("<p class=\"js-tweet-text tweet-text\" lang=\"ru\" data-aria-label-part=\"0\">Идеальный <a href=\"http://t.co/a44WljjX1x\" rel=\"nofollow\" dir=\"ltr\" data-expanded-url=\"https://weather.yahoo.com/\" class=\"twitter-timeline-link\" target=\"_blank\" title=\"https://weather.yahoo.com/\" >weather.yahoo.com</a> осенний день в <a href=\"/hashtag/ekb?src=hash\"><b>#ekb</b></a> !</p>");
        const HT_TAG tagsSeq[] = {HT_HTML, HT_HEAD, HT_HEAD, HT_BODY, HT_P,
                                  HT_A, HT_any, HT_any, HT_any, HT_A, HT_A, HT_B, HT_any, HT_B, HT_A, HT_any,
                                  HT_P, HT_BODY, HT_HTML};

        CheckTweet(data, tagsSeq);
    }

    Y_UNIT_TEST(HashLinkTweet) {
        const TStringBuf data = TStringBuf("<p class=\"js-tweet-text tweet-text\" lang=\"ru\" data-aria-label-part=\"0\">Идеальный осенний день в <a href=\"/hashtag/ekb?src=hash\" data-query-source=\"hashtag_click\" class=\"twitter-hashtag pretty-link js-nav\" dir=\"ltr\">#ekb</a> text <a href=\"http://t.co/a44WljjX1x\" rel=\"nofollow\" dir=\"ltr\" data-expanded-url=\"https://weather.yahoo.com/\" class=\"twitter-timeline-link\" target=\"_blank\" title=\"https://weather.yahoo.com/\" >weather.yahoo.com</a>  !</p>");
        const HT_TAG tagsSeq[] = {HT_HTML, HT_HEAD, HT_HEAD, HT_BODY, HT_P,
                                  HT_any, HT_A, HT_any, HT_A, HT_A, HT_any, HT_any, HT_any, HT_A,
                                  HT_P, HT_BODY, HT_HTML};

        CheckTweet(data, tagsSeq);
    }
}
