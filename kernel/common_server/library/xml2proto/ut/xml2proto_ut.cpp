#include <cstdint>
#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/str.h>
#include <util/string/builder.h>

#include <system_error>
#include <util/stream/output.h>
#include <library/cpp/testing/common/env.h>
#include <limits>

#include <kernel/common_server/library/xml2proto/ut/data/nbki.pb.h>
#include <kernel/common_server/library/xml2proto/ut/data/book.pb.h>
#include <kernel/common_server/library/xml2proto/converter.h>

namespace {
    TString TestDataPath(const TString& fileName) {
        return ArcadiaSourceRoot() + "/kernel/common_server/library/xml2proto/ut/data/" + fileName;
    }
}

Y_UNIT_TEST_SUITE(Xml2protoTest) {
    char* endptr = NULL;

    Y_UNIT_TEST(TestBookSimple) {
        NXml::TDocument xml(TestDataPath("book.xml"));

        NXml::TNode root = xml.Root();
        NBookProto::TBook proto;

        NProtobufXml::TConverter::XmlToProto(root, proto);

        auto authorNodes = root.XPath("Info/Author");
        UNIT_ASSERT_EQUAL(authorNodes.size(), 1);
        UNIT_ASSERT_STRINGS_EQUAL_C(authorNodes[0].Value<TString>(), proto.GetInfo().GetAuthor(), "Petr Smirnov");

        auto chaptersNumberNodes = root.XPath("Info/ChaptersNumber");
        UNIT_ASSERT_EQUAL(chaptersNumberNodes.size(), 1);
        UNIT_ASSERT_EQUAL_C(std::strtoul(chaptersNumberNodes[0].Value<TString>().c_str(), &endptr, 0), proto.GetInfo().GetChaptersNumber(), 2);

        auto pagesNumberNodes = root.XPath("Info/PagesNumber");
        UNIT_ASSERT_EQUAL(pagesNumberNodes.size(), 1);
        UNIT_ASSERT_EQUAL_C(std::strtoull(pagesNumberNodes[0].Value<TString>().c_str(), &endptr, 0), proto.GetInfo().GetPagesNumber(), 7);

        auto weightNodes = root.XPath("Info/Weight");
        UNIT_ASSERT_EQUAL(weightNodes.size(), 1);
        UNIT_ASSERT_DOUBLES_EQUAL_C((double)0.000001, (double)std::strtof(weightNodes[0].Value<TString>().c_str(), &endptr), (double)proto.GetInfo().GetWeight(), (double)0.543);

        auto scoreNodes = root.XPath("Score");
        UNIT_ASSERT_EQUAL(scoreNodes.size(), 1);
        UNIT_ASSERT_DOUBLES_EQUAL_C((double)0.000001 /*std::numeric_limits<double>::epsilon*/, (double)std::strtod(scoreNodes[0].Value<TString>().c_str(), &endptr), (double)proto.GetScore(), (double)9.86543);

        auto chapterNodes = root.XPath("Chapter");
        UNIT_ASSERT_EQUAL(chapterNodes.size(), 2);
        TSet<TString> xmlTitles;
        for (const auto& chapterNode : chapterNodes) {
            auto titleNodes = chapterNode.XPath("Title");
            UNIT_ASSERT_EQUAL(titleNodes.size(), 1);
            xmlTitles.insert(titleNodes[0].Value<TString>());
        }
        TSet<TString> protoTitles;
        for (const auto& chapter : proto.GetChapter()) {
            auto title = chapter.GetTitle();
            protoTitles.insert(title);
        }
        UNIT_ASSERT_EQUAL(xmlTitles, protoTitles);
        TSet<TString> expectedTitles{"chapter 1", "chapter 2"};
        UNIT_ASSERT_EQUAL(protoTitles, expectedTitles);

        auto pageNodes = root.XPath("Chapter/Page");
        UNIT_ASSERT_EQUAL(pageNodes.size(), 7);
        TSet<TString> xmlPages;
        for (const auto& pageNode : pageNodes) {
            xmlPages.insert(pageNode.Value<TString>());
        }
        TSet<TString> protoPages;
        for (const auto& chapter : proto.GetChapter()) {
            for (const auto& page : chapter.GetPage()) {
                protoPages.insert(page);
            }
        }
        UNIT_ASSERT_EQUAL(xmlTitles, protoTitles);
        TSet<TString> expectedPages{"page 1", "page 2", "page 3", "page 4", "page 5", "page 6", "page 7"};
        UNIT_ASSERT_EQUAL(protoPages, expectedPages);
    }

    Y_UNIT_TEST(TestBookAttr) {
        NXml::TDocument xml(TestDataPath("book_attr.xml"));

        NXml::TNode root = xml.Root();
        NBookProto::TBook proto;

        NProtobufXml::TConverter::XmlToProto(root, proto, true);

        UNIT_ASSERT_STRINGS_EQUAL(proto.GetInfo().GetAuthor(), "Petr Smirnov");
        UNIT_ASSERT_EQUAL(proto.GetInfo().GetChaptersNumber(), 2);
        UNIT_ASSERT_EQUAL(proto.GetInfo().GetPagesNumber(), 7);
        UNIT_ASSERT_DOUBLES_EQUAL((double)0.000001, (double)proto.GetInfo().GetWeight(), (double)0.543);
        UNIT_ASSERT_STRINGS_EQUAL(proto.GetInfo().GetId().GetType(), "ISBN");
        UNIT_ASSERT_STRINGS_EQUAL(proto.GetInfo().GetId().GetValue(), "2-266-11156");
        UNIT_ASSERT_DOUBLES_EQUAL((double)0.000001 /*std::numeric_limits<double>::epsilon*/, (double)proto.GetScore(), (double)9.86543);

        TSet<TString> protoTitles;
        for (const auto& chapter : proto.GetChapter()) {
            auto title = chapter.GetTitle();
            protoTitles.insert(title);
        }
        TSet<TString> expectedTitles{"chapter 1", "chapter 2"};
        UNIT_ASSERT_EQUAL(protoTitles, expectedTitles);

        TSet<TString> protoPages;
        for (const auto& chapter : proto.GetChapter()) {
            for (const auto& page : chapter.GetPage()) {
                protoPages.insert(page);
            }
        }
        TSet<TString> expectedPages{"page 1", "page 2", "page 3", "page 4", "page 5", "page 6", "page 7"};
        UNIT_ASSERT_EQUAL(protoPages, expectedPages);
    }

    Y_UNIT_TEST(TestNbkiResponse) {
        NXml::TDocument xml(TestDataPath("nbki.xml"));

        NXml::TNode root = xml.Root();
        auto productNodes = root.XPath("product");
        UNIT_ASSERT_EQUAL(productNodes.size(), 1);
        auto productNode = productNodes[0];
        NNBKIProto::ProductType proto;

        NProtobufXml::TConverter::XmlToProto(productNode, proto);

        auto nameNodes = root.XPath("product/prequest/req/PersonReq/name1");
        UNIT_ASSERT_EQUAL(nameNodes.size(), 1);
        UNIT_ASSERT_EQUAL_C(nameNodes[0].Value<TString>(), proto.Getprequest().Getreq().GetPersonReq().Getname1(), "Иванов");

        auto addressReqNodes = root.XPath("product/prequest/req/AddressReq");
        UNIT_ASSERT_EQUAL_C((size_t)addressReqNodes.size(), (size_t)proto.Getprequest().Getreq().GetAddressReq().size(), (size_t)2u);

        auto idReqNodes = root.XPath("product/prequest/req/IdReq");
        UNIT_ASSERT_EQUAL_C((size_t)idReqNodes.size(), (size_t)proto.Getprequest().Getreq().GetIdReq().size(), (size_t)1);
        auto idNumNodes = root.XPath("product/prequest/req/IdReq/idNum");
        UNIT_ASSERT_EQUAL((size_t)idNumNodes.size(), (size_t)1);
        UNIT_ASSERT_EQUAL_C(idNumNodes[0].Value<TString>(), proto.Getprequest().Getreq().GetIdReq()[0].GetidNum(), "123456");

        auto inqControlNumsNodes = root.XPath("product/preply/report/OwnInquiries/Inquiry");
        TSet<int64_t> xmlInqControlNums;
        char* endptr = NULL;
        for (const auto& inqControlNumsNode : inqControlNumsNodes) {
            xmlInqControlNums.insert(std::strtoll(inqControlNumsNode.Value<TString>().c_str(), &endptr, 0));
        }
        TSet<int64_t> protoInqControlNums;
        for (const auto& inquiry : proto.Getpreply().Getreport().GetOwnInquiries().GetInquiry()) {
            protoInqControlNums.insert(inquiry.GetinqControlNum());
        }
        UNIT_ASSERT_EQUAL(xmlInqControlNums, protoInqControlNums);
        TSet<int64_t> ExpectedInqControlNums{2120177617, 2120176886, 2120114207, 2120112695, 2120112126, 2016752601, 2016749034, 2009457362, 2009455724, 2008474179, 2006992348, 2000914558, 2000911021, 2000032466, 1996502878, 1970652930, 1970646438, 1965236987, 1962769566, 1957854885, 1957033972, 1951157774, 1949546930, 1949545054, 1934192989, 1862823649, 1862628199, 1854030998, 1853876957, 1838800659, 1838794947, 1812593444, 1812111385, 1687841417, 1289583887};
        UNIT_ASSERT_EQUAL(protoInqControlNums, ExpectedInqControlNums);

    }

    Y_UNIT_TEST(Empty) {
        NBookProto::TBook proto;
        auto info = proto.MutableInfo();
        info->SetAuthor("Petr Smirnov");
        info->SetChaptersNumber(2);
        info->SetPagesNumber(7);
        info->SetWeight(0.543);
        auto chapter1 = proto.AddChapter();
        chapter1->SetTitle("chapter 1");
        chapter1->AddPage("page 1");
        chapter1->AddPage("page 2");
        chapter1->AddPage("page 3");
        auto chapter2 = proto.AddChapter();
        chapter2->SetTitle("chapter 2");
        chapter2->AddPage("page 4");
        chapter2->AddPage("page 5");
        chapter2->AddPage("page 6");
        chapter2->AddPage("page 7");
        proto.SetScore(9.86543);

        NXml::TDocument doc("Book", NXml::TDocument::Source::RootName);
        NXml::TNode root = doc.Root();

        NProtobufXml::TConverter::ProtoToXml(proto, root);

        auto authorNodes = root.XPath("Info/Author");
        UNIT_ASSERT_EQUAL(authorNodes.size(), 1);
        UNIT_ASSERT_STRINGS_EQUAL_C(authorNodes[0].Value<TString>(), proto.GetInfo().GetAuthor(), "Petr Smirnov");

        auto chaptersNumberNodes = root.XPath("Info/ChaptersNumber");
        UNIT_ASSERT_EQUAL(chaptersNumberNodes.size(), 1);
        UNIT_ASSERT_EQUAL_C(std::strtoul(chaptersNumberNodes[0].Value<TString>().c_str(), &endptr, 0), proto.GetInfo().GetChaptersNumber(), 2);

        auto pagesNumberNodes = root.XPath("Info/PagesNumber");
        UNIT_ASSERT_EQUAL(pagesNumberNodes.size(), 1);
        UNIT_ASSERT_EQUAL_C(std::strtoull(pagesNumberNodes[0].Value<TString>().c_str(), &endptr, 0), proto.GetInfo().GetPagesNumber(), 7);

        auto weightNodes = root.XPath("Info/Weight");
        UNIT_ASSERT_EQUAL(weightNodes.size(), 1);
        UNIT_ASSERT_DOUBLES_EQUAL_C((double)0.000001, (double)std::strtof(weightNodes[0].Value<TString>().c_str(), &endptr), (double)proto.GetInfo().GetWeight(), (double)0.543);

        auto scoreNodes = root.XPath("Score");
        UNIT_ASSERT_EQUAL(scoreNodes.size(), 1);
        UNIT_ASSERT_DOUBLES_EQUAL_C((double)0.000001 /*std::numeric_limits<double>::epsilon*/, (double)std::strtod(scoreNodes[0].Value<TString>().c_str(), &endptr), (double)proto.GetScore(), (double)9.86543);

        auto chapterNodes = root.XPath("Chapter");
        UNIT_ASSERT_EQUAL(chapterNodes.size(), 2);
        TSet<TString> xmlTitles;
        for (const auto& chapterNode : chapterNodes) {
            auto titleNodes = chapterNode.XPath("Title");
            UNIT_ASSERT_EQUAL(titleNodes.size(), 1);
            xmlTitles.insert(titleNodes[0].Value<TString>());
        }
        TSet<TString> protoTitles;
        for (const auto& chapter : proto.GetChapter()) {
            auto title = chapter.GetTitle();
            protoTitles.insert(title);
        }
        UNIT_ASSERT_EQUAL(xmlTitles, protoTitles);
        TSet<TString> expectedTitles{"chapter 1", "chapter 2"};
        UNIT_ASSERT_EQUAL(protoTitles, expectedTitles);

        auto pageNodes = root.XPath("Chapter/Page");
        UNIT_ASSERT_EQUAL(pageNodes.size(), 7);
        TSet<TString> xmlPages;
        for (const auto& pageNode : pageNodes) {
            xmlPages.insert(pageNode.Value<TString>());
        }
        TSet<TString> protoPages;
        for (const auto& chapter : proto.GetChapter()) {
            for (const auto& page : chapter.GetPage()) {
                protoPages.insert(page);
            }
        }
        UNIT_ASSERT_EQUAL(xmlTitles, protoTitles);
        TSet<TString> expectedPages{"page 1", "page 2", "page 3", "page 4", "page 5", "page 6", "page 7"};
        UNIT_ASSERT_EQUAL(protoPages, expectedPages);

    }
}
