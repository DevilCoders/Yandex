#include <library/cpp/html/face/onchunk.h>
#include <library/cpp/html/face/noindex.h>
#include <library/cpp/html/html5/parse.h>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TDetectNoindex) {
    Y_UNIT_TEST(IsNoindexOpen) {
        UNIT_ASSERT(true == NHtml5::DetectNoindex("noindex").IsNoindex());
        UNIT_ASSERT(true == NHtml5::DetectNoindex("NoIndeX").IsNoindex());
        UNIT_ASSERT(false == NHtml5::DetectNoindex("n0index").IsNoindex());
        UNIT_ASSERT(false == NHtml5::DetectNoindex("just noindex it").IsNoindex());
        UNIT_ASSERT(false == NHtml5::DetectNoindex("").IsNoindex());

        UNIT_ASSERT(false == NHtml5::DetectNoindex("noindex").IsClose());
        UNIT_ASSERT(false == NHtml5::DetectNoindex("NoIndeX").IsClose());
    }

    Y_UNIT_TEST(IsNoindexClose) {
        UNIT_ASSERT(true == NHtml5::DetectNoindex("/noindex").IsNoindex());
        UNIT_ASSERT(true == NHtml5::DetectNoindex("/ noindex").IsNoindex());
        UNIT_ASSERT(false == NHtml5::DetectNoindex("//noindex").IsNoindex());
        UNIT_ASSERT(true == NHtml5::DetectNoindex("  / noindex ").IsNoindex());
        UNIT_ASSERT(false == NHtml5::DetectNoindex("noindex/").IsNoindex());
        UNIT_ASSERT(false == NHtml5::DetectNoindex("/").IsNoindex());

        UNIT_ASSERT(true == NHtml5::DetectNoindex("/noindex").IsClose());
        UNIT_ASSERT(true == NHtml5::DetectNoindex("  / noindex ").IsClose());
    }

    Y_UNIT_TEST(EnableNoindex) {
        class TNoindexHandle: public IParserResult {
        public:
            TNoindexHandle()
                : Has_(false)
            {
            }

            THtmlChunk* OnHtmlChunk(const THtmlChunk& chunk) override {
                if (chunk.flags.type == PARSED_TEXT) {
                    Has_ |= (chunk.flags.weight == WEIGHT_ZERO) && (chunk.Format & IRREG_NOINDEX);
                }
                return nullptr;
            }

            TNoindexHandle& Parse(const TStringBuf& html, bool enableNoindex) {
                NHtml5::TParserOptions opts;
                opts.EnableNoindex = enableNoindex;
                NHtml5::ParseHtml(html, this, opts);
                return *this;
            }

            bool IsZeroWeight() const {
                return Has_;
            }

        private:
            bool Has_;
        };

        const char* html = "<body><noindex>text</noindex></body>";

        UNIT_ASSERT(true == TNoindexHandle().Parse(html, true).IsZeroWeight());
        UNIT_ASSERT(false == TNoindexHandle().Parse(html, false).IsZeroWeight());
    }
}
