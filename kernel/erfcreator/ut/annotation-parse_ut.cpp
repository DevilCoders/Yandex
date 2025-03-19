#include <kernel/erfcreator/orangeattrs.h>
#include <kernel/indexer/faceproc/docattrs.h> //for TFullDocAttrs

#include <yweb/protos/robot.pb.h> // for TAnchorText
#include <yweb/protos/indexeddoc.pb.h> //for TIndexedDoc

#include <library/cpp/testing/unittest/registar.h>

using namespace NOrangeData;

Y_UNIT_TEST_SUITE(AnnotationParseTestSuite) {

    using namespace NRealTime;
    typedef google::protobuf::RepeatedPtrField<TDocAnnotation> TAnnotations;

    void FillBlogAnnotation(const TString& source, const TString& type, const TString& value, TDocAnnotation* res) {
        res->SetSource(source);
        res->SetType(type);
        res->AddValues(value);
        res->SetModTime(1317398321); // simple const modTime
    }

    TSimpleSharedPtr<TAnnotationProcessor> DoProcess(
        const TString& prefferedSource,
        ui64 erfUrlHash,
        const TAnnotations& annotations,
        TFullDocAttrs& docAttrs,
        TIndexedDoc& indexedDoc)
    {
        THolder<TAnnotationProcessor> annotationProcessor(new TAnnotationProcessor(prefferedSource, /*useErfCreator =*/ true));
        TAnchorText anchorText;
        bool useNewsAnnotationParser = true;
        annotationProcessor->Process(annotations, useNewsAnnotationParser, docAttrs, indexedDoc, &anchorText);
        annotationProcessor->SetUniqueHashes(erfUrlHash, indexedDoc);

        return annotationProcessor.Release();
    }

    void SimpleBlogAnnotationCheck(
        const TAnnotations& annotations,
        const TString& blogCompareString,
        size_t badSnippetsNum = 0,
        const char* attrName = "posts")
    {
        TFullDocAttrs docAttrs;
        TIndexedDoc indexedDoc;

        TSimpleSharedPtr<TAnnotationProcessor> annotationProcessor(DoProcess("blogs_content", 0, annotations, docAttrs, indexedDoc));

        UNIT_ASSERT_EQUAL(annotationProcessor->GetBadSnippetsNum(), badSnippetsNum);
        if (!badSnippetsNum) {
            UNIT_ASSERT_EQUAL(std::distance(docAttrs.Begin(), docAttrs.End()), 1);

            const TFullDocAttrs::TAttr& attr = *docAttrs.Begin();
            UNIT_ASSERT_EQUAL(attr.Name, attrName);
            if (!!blogCompareString)
                UNIT_ASSERT_EQUAL(attr.Value, blogCompareString);
        }
    }

    Y_UNIT_TEST(SimpleBlogAnnotationTest) {
        TAnnotations annotations;
        TString blogCompareString =  "ft=blog\007;date=2317397647\007;author=rusanalit";
        TString blogAnnotationString = TString("posts=") + blogCompareString + "\t29365\t5534";

        FillBlogAnnotation("blogs_urls", "snippets", blogAnnotationString, annotations.Add());

        SimpleBlogAnnotationCheck(annotations, blogCompareString);
    }

    Y_UNIT_TEST(BlogAnnotations) {
        TAnnotations annotations;

        TString blogCompareString = "ft=blog\007;time=1317397647\007;title=ðÏÞÅÍÕ åò - ÐÁÒÔÉÑ ÖÕÌÉËÏ× É ×ÏÒÏ×\007;author=ôÅÏÒÉÑ ÚÁÇÏ×ÏÒÁ";
        TString blogAnnotationString = TString("posts=") + blogCompareString + "\t29365\t5534";

        FillBlogAnnotation("blogs_content", "snippets", blogAnnotationString, annotations.Add());
        FillBlogAnnotation("blogs_urls",  "snippets", "posts=ft=blog\007;date=2317397647\007;author=rusanalit\t29365\t5534", annotations.Add());

        SimpleBlogAnnotationCheck(annotations, blogCompareString);
    }

    Y_UNIT_TEST(BlogWrongAnnotationTest) {
        TAnnotations annotations;
        TString blogCompareString = "ft=blog\007;date=2317397647\007;author=rusanalit";
        FillBlogAnnotation("blogs_urls", "snippets", blogCompareString, annotations.Add());
        SimpleBlogAnnotationCheck(annotations, "", /*badSnippetsNum = */ 0, /*attrName = */ "ft");
    }

    Y_UNIT_TEST(UrlHashTest) {
        TAnnotations annotations;

        TDocAnnotation& ann = *annotations.Add();
        ann.SetType("urlhash");
        ann.SetModTime(1317729182);

        size_t hashesNumber = 100;
        for (size_t i = 0; i < hashesNumber; ++i) {
            // add twice - to check unique hashes
            ann.AddValues(ToString(i));
            ann.AddValues(ToString(i));
        }

        ann.AddValues("wrong_number");

        ui64 erfInfoHash = 99;

        TFullDocAttrs docAttrs;
        TIndexedDoc indexedDoc;
        TSimpleSharedPtr<TAnnotationProcessor> annotationProcessor(DoProcess("some_source", erfInfoHash, annotations, docAttrs, indexedDoc));

        UNIT_ASSERT_EQUAL(indexedDoc.UrlHashesSize(), hashesNumber - 1); // one hash in erfInfo
        UNIT_ASSERT_EQUAL(annotationProcessor->GetBadUrlHashesNum(), 1);
    }

    void PrepareNavigational(const char** testLines, google::protobuf::RepeatedPtrField<TDocAnnotation>& annotations) {
        for(size_t i = 0; testLines[i]; i++) {
            TDocAnnotation& ann = *annotations.Add();
            ann.SetType("navigational");
            ann.AddValues(testLines[i]);
            ann.SetModTime(1317729182);
        }
    }

    void CheckNavigationalResult(const char** results4Compare, const NRobot::TNavInfo& navInfo) {
        for(size_t i = 0; i < navInfo.NavItemsSize(); ++i) {
            const NRobot::TNavInfo::TNavItem& item = navInfo.GetNavItems(i);
            size_t answerIndex = i * 2;

            UNIT_ASSERT_EQUAL(item.GetQuery(), TString(results4Compare[answerIndex]));
            UNIT_ASSERT_EQUAL(item.GetAdHoc(), TString(results4Compare[answerIndex + 1]));
        }
    }

    void ProcessNavigationalTest(
        const char** testLines,
        const char** canonicalResilts,
        const size_t resultsCount,
        const size_t badCount)
    {
        TAnnotations annotations;
        PrepareNavigational(testLines, annotations);

        TFullDocAttrs docAttrs;
        TIndexedDoc indexedDoc;
        TSimpleSharedPtr<TAnnotationProcessor> annotationProcessor(DoProcess("some_source", 0, annotations, docAttrs, indexedDoc));

        UNIT_ASSERT_EQUAL(annotationProcessor->GetBadNavigationalsNum(), badCount);
        UNIT_ASSERT_EQUAL(indexedDoc.GetNavInfoUtf().NavItemsSize(), resultsCount);
        CheckNavigationalResult(canonicalResilts, indexedDoc.GetNavInfoUtf());
    }

    Y_UNIT_TEST(SimpleNavigationalTest) {
        const char* testNavigationalLines[] = {
            "query=yandex\\\\rr,adhoc=1\t0;",
            "query=yandex,adhoc=1\t0;query=google,adhoc=j0\t-1",
            "query=word\\\\t,adhoc=1\t0",
            nullptr
        };

        const char* results4Compare[] = {
            "yandex\\rr", "1\t0",
            "yandex", "1\t0",
            "google", "j0\t-1",
            "word\\t", "1\t0"
        };
        ProcessNavigationalTest(testNavigationalLines, results4Compare, Y_ARRAY_SIZE(results4Compare) / 2, /*badCount = */0);
    }

    Y_UNIT_TEST(SecondNavigationalTest) {
        const char* testNavigationalLines[] = {
            "query=\"text1;text1\",adhoc=\"123,567\";",
            "query=\"text2\",adhoc=\"89\"",
            nullptr
        };

        const char* results4Compare[] = {
            "text1;text1", "123,567",
            "text2", "89"
        };

        ProcessNavigationalTest(testNavigationalLines, results4Compare, Y_ARRAY_SIZE(results4Compare) / 2, /*badCount = */0);
    }

    Y_UNIT_TEST(DifferentOrderTest) {
        const char* testNavigationalLines[] = {
            "adhoc=\"123,567\",query=\"text1;text1\"",
            "adhoc=\"89\",query=\"text2\"",
            nullptr
        };

        const char* results4Compare[] = {
            "text1;text1", "123,567",
            "text2", "89"
        };

        ProcessNavigationalTest(testNavigationalLines, results4Compare, Y_ARRAY_SIZE(results4Compare) / 2, /*badCount = */0);
    }

    Y_UNIT_TEST(EscapedInQoutesTest) {
        const char* testNavigationalLines[] = {
            "query=\"AA\\\"BB\",adhoc=1\\t-1",
            nullptr
        };

        const char* results4Compare[] = {
            "AA\"BB", "1\t-1"
        };

        ProcessNavigationalTest(testNavigationalLines, results4Compare, Y_ARRAY_SIZE(results4Compare) / 2, /*badCount = */0);
    }

    Y_UNIT_TEST(UnclosedQuoteTest) {
        const char* testNavigationalLines[] = {
            "\"query=\"AA\\\"BB\",adhoc=1\\t-1",
            "adhoc=1\\t-1,query=\"AA\\\"BB",
            nullptr
        };

        const char* results4Compare[] = {
            "AA\"BB", "1\t-1"
        };

        ProcessNavigationalTest(testNavigationalLines, results4Compare, 0, /*badCount = */2);
    }

    Y_UNIT_TEST(TwoDelimitersAtOnceTest) {
        const char* testNavigationalLines[] = {
            "query=,QQ,adhoc=1\\t-1",
            nullptr
        };

        const char** results4Compare = nullptr; // empty result for compare
        ProcessNavigationalTest(testNavigationalLines, results4Compare, 0, /*badCount = */1);
    }

    Y_UNIT_TEST(EscapedDelimiterTest) {
        const char* testNavigationalLines[] = {
            "query=\\,QQ,adhoc=1\\t-1",
            nullptr
        };

        const char* results4Compare[] = {
            ",QQ", "1\t-1"
        };

        ProcessNavigationalTest(testNavigationalLines, results4Compare, Y_ARRAY_SIZE(results4Compare) / 2, /*badCount = */0);
    }

    Y_UNIT_TEST(BadAnnotationDoubleFieldTest) {
        const char* testNavigationalLines[] = {
            "adhoc=1\\t-1,adhoc=1\\t-1;query=\\,QQ,adhoc=1\\t-1",
            nullptr
        };

        const char** results4Compare = nullptr; // empty result for compare

        ProcessNavigationalTest(testNavigationalLines, results4Compare, 0, /*badCount = */1);
    };
}

