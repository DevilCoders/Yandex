#include "schemaorg_viewer.h"

#include <kernel/snippets/config/config.h>

#include <kernel/snippets/schemaorg/creativework.h>
#include <kernel/snippets/schemaorg/proto/schemaorg.pb.h>
#include <kernel/snippets/schemaorg/sozluk_comments.h>
#include <kernel/snippets/schemaorg/product_offer.h>
#include <kernel/snippets/schemaorg/question/question.h>
#include <kernel/snippets/schemaorg/movie.h>
#include <kernel/snippets/schemaorg/software.h>
#include <kernel/snippets/schemaorg/youtube_channel.h>
#include <kernel/snippets/schemaorg/videoobj.h>
#include <kernel/snippets/schemaorg/rating.h>
#include <kernel/snippets/schemaorg/schemaorg_serializer.h>
#include <kernel/snippets/schemaorg/schemaorg_traversal.h>

#include <kernel/snippets/archive/unpacker/unpacker.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/archive/view/order.h>
#include <kernel/snippets/iface/archive/viewer.h>

#include <kernel/lemmer/alpha/abc.h>

#include <util/charset/unidata.h>
#include <util/charset/wide.h>
#include <util/generic/list.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/generic/singleton.h>
#include <util/string/strip.h>
#include <library/cpp/string_utils/url/url.h>

using namespace NSchemaOrg;

namespace NSnippets {
    class TNodeValueUnpackOptions {
    public:
        bool IsTypedNode;
        size_t MaxValueSentsCount;
        size_t MaxValueLength;

        TNodeValueUnpackOptions(bool isTypedNode, size_t maxValueSentsCount, size_t maxValueLength)
            : IsTypedNode(isTypedNode)
            , MaxValueSentsCount(maxValueSentsCount)
            , MaxValueLength(maxValueLength)
        {
        }
    };

    static const TNodeValueUnpackOptions LONG_TEXT(false, 40, 3000);
    static const TNodeValueUnpackOptions LONG_TEXT_RAW(true, 40, 3000);
    static const TNodeValueUnpackOptions SHORT_TEXT(false, 12, 300);
    static const TNodeValueUnpackOptions SHORT_TEXT_RAW(true, 12, 500);
    static const TNodeValueUnpackOptions TYPED_FIELD(true, 8, 150);

    class TNodeValueUnpack {
    public:
        TUtf16String* Value;
        TSentsOrder Order;
        TNodeValueUnpackOptions Options;
        const NLemmer::TAlphabetWordNormalizer* WordNormalizer;

        TNodeValueUnpack(TUtf16String* value, const TTreeNode* node, const TNodeValueUnpackOptions& options, const NLemmer::TAlphabetWordNormalizer* wordNormalizer)
            : Value(value)
            , Order()
            , Options(options)
            , WordNormalizer(wordNormalizer)
        {
            Value->clear();
            if (node) {
                if (options.IsTypedNode && node->HasDatetime()) {
                    *Value = UTF8ToWide(node->GetDatetime());
                    return;
                }
                if (node->HasText()) {
                    TUtf16String text = FormatSent(UTF8ToWide(node->GetText()));
                    if (text.size() <= Options.MaxValueLength) {
                        *Value = text;
                    }
                } else if (node->HasSentBegin() && node->HasSentCount()) {
                    size_t count = Min((size_t)node->GetSentCount(), Options.MaxValueSentsCount);
                    Order.PushBack(node->GetSentBegin(), node->GetSentBegin() + count - 1);
                }
            }
        }

        void OnUnpacked() {
            if (!Order.Empty()) {
                TArchiveView view;
                DumpResult(Order, view);
                TUtf16String value;
                for (size_t i = 0; i < view.Size(); ++i) {
                    TUtf16String text = FormatSent(view.Get(i)->Sent);
                    if (value.size() + (value.size() ? 1 : 0) + text.size() >= Options.MaxValueLength) {
                        break;
                    }
                    if (value.size()) {
                        value += ' ';
                    }
                    value += text;
                }
                *Value = StripString(value);
                Order.Clear();
            }
        }

    private:
        TUtf16String FormatSent(const TWtringBuf& sent) {
            if (!sent) {
                return TUtf16String();
            }
            if (Options.IsTypedNode) {
                return ToWtring(sent);
            }

            TUtf16String result;
            result += WordNormalizer->ToUpper(sent[0]);
            result += sent.SubStr(1);
            if (!IsTerminal(result.back())) {
                result += '.';
            }
            return result;
        }
    };

    class TSchemaOrgArchiveViewer::TImpl : public IArchiveViewer {
    public:
        const TConfig& Cfg;
        const TDocInfos& DocInfos;
        const TString Url;
        const NLemmer::TAlphabetWordNormalizer* WordNormalizer;
        TUnpacker* Unpacker;
        TVector<TAutoPtr<TNodeValueUnpack>> Unpacks;
        THolder<TOffer> Offer;
        THolder<TMovie> Movie;
        THolder<TRating> Rating;
        THolder<TSozlukComments> EksisozlukComments;
        THolder<TYoutubeChannel> YoutubeChannel;
        THolder<TCreativeWork> CreativeWork;
        THolder<TQuestion> Question;
        THolder<TSoftwareApplication> SoftwareApplication;
        TTreeNode BestAnswerLabelTextNode;

        TImpl(const TConfig& cfg, const TDocInfos& docInfos, const TString& url, ELanguage lang)
            : Cfg(cfg)
            , DocInfos(docInfos)
            , Url(url)
            , WordNormalizer(NLemmer::GetAlphaRules(lang))
        {
        }

        void OnUnpacker(TUnpacker* unpacker) override {
            Unpacker = unpacker;
            auto it = DocInfos.find("SchemaOrg");
            if (it == DocInfos.end()) {
                return;
            }
            TStringBuf serializedSchemaOrg(it->second);
            TTreeNode root;
            if (!DeserializeFromBase64(serializedSchemaOrg, root)) {
                return;
            }
            UnpackOffer(&root);
            UnpackMovie(&root);
            UnpackRating(&root);
            TStringBuf host = GetOnlyHost(Url);
            if (host == "eksisozluk.com") {
                UnpackEksisozlukComments(&root);
            }
            UnpackYoutubeChannel(&root);
            UnpackCreativeWork(&root, NeedVideoThumb(host));
            UnpackQuestion(&root, host == "otvet.mail.ru");
            UnpackSoftwareApplication(&root);
        }

        void OnEnd() override {
            for (auto& unpack : Unpacks) {
                unpack->OnUnpacked();
            }
            Unpacks.clear();
        }

    private:
        void UnpackField(TUtf16String* field, const TTreeNode* node, const TNodeValueUnpackOptions& options) {
            TAutoPtr<TNodeValueUnpack> unpack(new TNodeValueUnpack(field, node, options, WordNormalizer));
            Unpacker->AddRequest(unpack->Order);
            Unpacks.push_back(unpack);
        }

        void UnpackList(TList<TUtf16String>& fields, const TList<const TTreeNode*>& propNodes, const TNodeValueUnpackOptions& options) {
            for (const TTreeNode* node : propNodes) {
                fields.push_back(TUtf16String());
                UnpackField(&fields.back(), node, options);
            }
        }

        void UnpackOffer(const TTreeNode* root) {
            const TTreeNode* productNode = FindSingleItemtype(root, "product");
            if (!productNode) {
                return;
            }
            Offer.Reset(new TOffer());
            const TTreeNode* aggregateOfferNode = nullptr;
            const TTreeNode* offerNode = nullptr;
            TList<const TTreeNode*> offerNodes = FindAllItemprops(productNode, "offers", false);
            // deny Product without offers
            if (offerNodes.empty()) {
                Offer->ErrorMessage = "Product/offers field is missing";
                return;
            }
            // accept single AggregateOffer ignoring all other offers
            for (const TTreeNode* node : offerNodes) {
                if (IsItemOfType(*node, "aggregateoffer")) {
                    if (aggregateOfferNode) {
                        Offer->ErrorMessage = "multiple Product/AggregateOffer fields";
                        return;
                    }
                    aggregateOfferNode = node;
                }
            }
            // accept single Offer
            if (!aggregateOfferNode && offerNodes.size() == 1) {
                offerNode = offerNodes.back();
            }
            UnpackField(&Offer->ProductName, FindFirstItemprop(productNode, "name"), SHORT_TEXT);
            UnpackList(Offer->ProductDesc, FindAllItemprops(productNode, "description", false), LONG_TEXT);
            if (aggregateOfferNode) {
                UnpackList(Offer->OfferDesc, FindAllItemprops(aggregateOfferNode, "description", false), LONG_TEXT);
                UnpackField(&Offer->LowPrice, FindSingleItemprop(aggregateOfferNode, "lowprice"), TYPED_FIELD);
                UnpackField(&Offer->PriceCurrency, FindSingleItemprop(aggregateOfferNode, "pricecurrency"), TYPED_FIELD);
            }
            if (offerNode) {
                UnpackList(Offer->OfferDesc, FindAllItemprops(offerNode, "description", false), LONG_TEXT);
                UnpackField(&Offer->Availability, FindFirstItemprop(offerNode, "availability"), TYPED_FIELD);
                UnpackList(Offer->AvailableAtOrFrom, FindAllItemprops(FindSingleItemprop(offerNode, "availableatorfrom"), "name", false), SHORT_TEXT);
                UnpackField(&Offer->ValidThrough, FindFirstItemprop(offerNode, "validthrough"), TYPED_FIELD);
                UnpackField(&Offer->PriceValidUntil, FindFirstItemprop(offerNode, "pricevaliduntil"), TYPED_FIELD);
                UnpackField(&Offer->Price, FindSingleItemprop(offerNode, "price"), TYPED_FIELD);
                UnpackField(&Offer->PriceCurrency, FindSingleItemprop(offerNode, "pricecurrency"), TYPED_FIELD);
            }
        }

        void UnpackMovie(const TTreeNode* root) {
            const TTreeNode* movieNode = FindSingleItemtype(root, "movie");
            if (!movieNode) {
                return;
            }

            Movie.Reset(new TMovie());
            UnpackList(Movie->Name, FindAllItemprops(movieNode, "name", false), SHORT_TEXT_RAW);
            const TTreeNode* ratingNode = FindSingleItemtype(movieNode, "aggregaterating");
            if (ratingNode) {
                UnpackField(&Movie->RatingValue, FindFirstItemprop(ratingNode, "ratingvalue"), TYPED_FIELD);
                UnpackField(&Movie->BestRating, FindFirstItemprop(ratingNode, "bestrating"), TYPED_FIELD);
            }
            UnpackList(Movie->Genre, ReplaceItemtypesWithNames(FindAllItemprops(movieNode, "genre", true)), SHORT_TEXT_RAW);
            UnpackList(Movie->Director, ReplaceItemtypesWithNames(FindAllItemprops(movieNode, "director", true)), SHORT_TEXT_RAW);
            UnpackList(Movie->Actor, ReplaceItemtypesWithNames(FindAllItemprops(movieNode, "actor", true)), SHORT_TEXT_RAW);
            UnpackField(&Movie->Description, FindFirstItemprop(movieNode, "description"), LONG_TEXT);
            UnpackField(&Movie->Duration, FindFirstItemprop(movieNode, "duration"), TYPED_FIELD);
            UnpackField(&Movie->InLanguage, FindFirstItemprop(movieNode, "inlanguage"), TYPED_FIELD);
            UnpackList(Movie->MusicBy, ReplaceItemtypesWithNames(FindAllItemprops(movieNode, "musicby", false)), SHORT_TEXT_RAW);
            UnpackList(Movie->Producer, ReplaceItemtypesWithNames(FindAllItemprops(movieNode, "producer", true)), SHORT_TEXT_RAW);
        }

        void UnpackRating(const TTreeNode* root) {
            TVector<const TTreeNode*> ratingNodes;
            TVector<const TTreeNode*> aggregateRatingNodes;
            TraverseTree(root, [&](const TTreeNode& node) -> bool {
                if (IsItemOfType(node, "rating")) {
                    ratingNodes.push_back(&node);
                }
                if (IsItemOfType(node, "aggregaterating")) {
                    aggregateRatingNodes.push_back(&node);
                }
                return true;
            });

            const TTreeNode* ratingNode = nullptr;
            if (aggregateRatingNodes.size() == 1) {
                ratingNode = aggregateRatingNodes[0];
            } else if (!aggregateRatingNodes && ratingNodes.size() == 1) {
                ratingNode = ratingNodes[0];
            } else {
                return;
            }

            Rating.Reset(new TRating());
            UnpackField(&Rating->RatingValue, FindFirstItemprop(ratingNode, "ratingvalue"), TYPED_FIELD);
            UnpackField(&Rating->BestRating, FindFirstItemprop(ratingNode, "bestrating"), TYPED_FIELD);
            UnpackField(&Rating->WorstRating, FindFirstItemprop(ratingNode, "worstrating"), TYPED_FIELD);
            UnpackField(&Rating->RatingCount, FindFirstItemprop(ratingNode, "ratingcount"), TYPED_FIELD);
        }

        void UnpackEksisozlukComments(const TTreeNode* root) {
            const TTreeNode* webPageNode = FindSingleItemtype(root, "webpage");
            TList<const TTreeNode*> commentNodes = FindAllItemprops(webPageNode, "comment", false);
            TList<const TTreeNode*> commentTextNodes;
            for (const TTreeNode* commentNode : commentNodes) {
                const TTreeNode* node = FindSingleItemprop(commentNode, "commenttext");
                if (node) {
                    commentTextNodes.push_back(node);
                }
            }
            if (!commentTextNodes.empty()) {
                EksisozlukComments.Reset(new TSozlukComments());
                UnpackList(EksisozlukComments->Comments, commentTextNodes, LONG_TEXT_RAW);
            }
        }

        void UnpackYoutubeChannel(const TTreeNode* root) {
            const TTreeNode* node = FindSingleItemtype(root, "youtubechannelv2");
            if (!node)
                return;

            YoutubeChannel.Reset(new TYoutubeChannel());
            UnpackField(&YoutubeChannel->Name, FindFirstItemprop(node, "name"), TYPED_FIELD);
            UnpackField(&YoutubeChannel->Description, FindFirstItemprop(node, "description"), LONG_TEXT);
            UnpackField(&YoutubeChannel->IsFamilyFriendly, FindFirstItemprop(node, "isfamilyfriendly"), TYPED_FIELD);
        }

        void UnpackCreativeWork(const TTreeNode* root, bool needVThumb) {
            const char* const ITEMTYPES_PRIORITY[] = {
                "tvseries", "tvseason", "tvepisode", "book",
                "videoobject", "audioobject", "musicvideoobject", "imageobject", "mediaobject",
                "blogposting", "blog",
                "medicalscholarlyarticle", "scholarlyarticle", "newsarticle", "article",
                "exerciseplan", "diet",
                "photograph", "painting", "sculpture",
                "creativework",
            };
            const TTreeNode* typeRoot = nullptr;
            for (const auto itemtype : ITEMTYPES_PRIORITY) {
                typeRoot = FindSingleItemtype(root, itemtype);
                if (typeRoot) {
                    break;
                }
            }
            if (!typeRoot) {
                return;
            }
            CreativeWork.Reset(new TCreativeWork());
            UnpackField(&CreativeWork->Name, FindFirstItemprop(typeRoot, "name"), SHORT_TEXT_RAW);
            UnpackField(&CreativeWork->Headline, FindFirstItemprop(typeRoot, "headline"), SHORT_TEXT_RAW);
            UnpackField(&CreativeWork->Description, FindFirstItemprop(typeRoot, "description"), LONG_TEXT);
            UnpackField(&CreativeWork->ArticleBody, FindFirstItemprop(typeRoot, "articlebody"), LONG_TEXT);
            if (IsItemOfType(*typeRoot, "book")) {
                UnpackList(CreativeWork->Author, ReplaceItemtypesWithNames(FindAllItemprops(typeRoot, "author", true)), SHORT_TEXT_RAW);
            }
            if (IsItemOfType(*typeRoot, "tvepisode")
                || IsItemOfType(*typeRoot, "tvseason")
                || IsItemOfType(*typeRoot, "tvseries"))
            {
                UnpackList(CreativeWork->Genre, ReplaceItemtypesWithNames(FindAllItemprops(typeRoot, "genre", true)), SHORT_TEXT_RAW);
            }
            if (needVThumb && IsItemOfType(*typeRoot, "videoobject")) {
                UnpackField(&CreativeWork->VideoDuration, FindSingleItemprop(typeRoot, "duration"), TYPED_FIELD);
                const TTreeNode* thumbNode = FindSingleItemprop(typeRoot, "thumbnailurl");
                if (thumbNode && thumbNode->HasHref()) {
                    CreativeWork->VideoThumbUrl = UTF8ToWide(thumbNode->GetHref());
                }
            }
        }

        void UnpackAuthorName(TUtf16String* res, const TTreeNode* elem) {
            const TTreeNode* const author = FindFirstItemprop(elem, "author");
            if (author) {
                UnpackField(res, FindFirstItemprop(author, "name"), SHORT_TEXT);
            }
        }

        void UnpackQuestion(const TTreeNode* root, bool isOtvetMailRu) {
            const int MAX_UNPACK_ANSWERS = 5;
            const TTreeNode* question = FindSingleItemtype(root, "question");
            if (!question) {
                return;
            }
            Question.Reset(new TQuestion());
            UnpackField(&Question->QuestionText, FindFirstItemprop(question, "name"), LONG_TEXT);
            UnpackAuthorName(&Question->AuthorName, question);
            UnpackField(&Question->DatePublished, FindFirstItemprop(question, "datepublished"), TYPED_FIELD);
            UnpackField(&Question->AnswerCount, FindFirstItemprop(question, "answercount"), TYPED_FIELD);
            const TTreeNode* acceptedAnswer = FindFirstItemprop(question, "acceptedanswer");
            if (acceptedAnswer) {
                Question->AcceptedAnswer.IsAccepted = true;
                UnpackField(&Question->AcceptedAnswer.Text, FindFirstItemprop(acceptedAnswer, "text"), LONG_TEXT);
                UnpackAuthorName(&Question->AcceptedAnswer.AuthorName, acceptedAnswer);
            }

            TList<const TTreeNode*> suggestedAnswers =
                FindAllItemprops(question, "suggestedanswer", false);
            if (isOtvetMailRu) {
                if (suggestedAnswers && suggestedAnswers.front()->HasSentBegin()) {
                    ui32 firstAnswerBegin = suggestedAnswers.front()->GetSentBegin();
                    if (firstAnswerBegin > 1) {
                        BestAnswerLabelTextNode.AddItemprops("text");
                        BestAnswerLabelTextNode.SetSentBegin(firstAnswerBegin - 1);
                        BestAnswerLabelTextNode.SetSentCount(1);
                        UnpackField(&Question->BestAnswerLabelTextNode, &BestAnswerLabelTextNode, SHORT_TEXT_RAW);
                    }
                }
            }
            if (suggestedAnswers.size() > MAX_UNPACK_ANSWERS) {
                suggestedAnswers.resize(MAX_UNPACK_ANSWERS);
            }
            for (const TTreeNode* answerNode : suggestedAnswers) {
                Question->SuggestedAnswer.push_back(TAnswer());
                TAnswer& answer = Question->SuggestedAnswer.back();
                UnpackField(&answer.Text, FindFirstItemprop(answerNode, "text"), LONG_TEXT);
                UnpackAuthorName(&answer.AuthorName, answerNode);
                TStringBuf upvoteCount;
                TryUnpackFieldText(answerNode, "upvotecount", upvoteCount);
                answer.SetUpvoteCount(upvoteCount);
            }
        }

        void TryUnpackFieldText(const TTreeNode* answerNode, const TString& fieldName, TStringBuf& target) {
            const TTreeNode* node = FindFirstItemprop(answerNode, fieldName);
            if (node && node->HasText()) {
                target = node->GetText();
            }
        }

        void UnpackSoftwareApplication(const TTreeNode* root) {
            const char* const ITEMTYPES_PRIORITY[] = {
                "softwareapplication",
                "mobileapplication",
                "webapplication",
                "videogame",
                "mobilesoftwareapplication",
            };
            const TTreeNode* typeRoot = nullptr;
            for (const auto itemtype : ITEMTYPES_PRIORITY) {
                typeRoot = FindSingleItemtype(root, itemtype);
                if (typeRoot) {
                    break;
                }
            }
            if (!typeRoot) {
                return;
            }

            SoftwareApplication.Reset(new TSoftwareApplication());
            UnpackField(&SoftwareApplication->Description, FindFirstItemprop(typeRoot, "description"), LONG_TEXT);
            UnpackList(SoftwareApplication->InteractionCount, FindAllItemprops(typeRoot, "interactioncount", false), TYPED_FIELD);
            UnpackField(&SoftwareApplication->FileSize, FindFirstItemprop(typeRoot, "filesize"), TYPED_FIELD);
            UnpackList(SoftwareApplication->OperatingSystem, FindAllItemprops(typeRoot, "operatingsystem", true), TYPED_FIELD);
            UnpackList(SoftwareApplication->ApplicationSubCategory, FindAllItemprops(typeRoot, "applicationsubcategory", false), TYPED_FIELD);

            const TTreeNode* offerNode = FindFirstItemprop(typeRoot, "offers");
            if (offerNode) {
                UnpackField(&SoftwareApplication->Price, FindSingleItemprop(offerNode, "price"), TYPED_FIELD);
                UnpackField(&SoftwareApplication->PriceCurrency, FindSingleItemprop(offerNode, "pricecurrency"), TYPED_FIELD);
            }
        }

        bool NeedVideoThumb(TStringBuf host) {
            if (!Cfg.UseSchemaVideoObj() || !Singleton<TVideoHostWhiteList>()->HasHost(host))
                return false;
            return !DocInfos.contains("vthumb"); // don't generate anything in case of the usual vthumb
        }
    };

    TSchemaOrgArchiveViewer::TSchemaOrgArchiveViewer(const TConfig& cfg, const TDocInfos& docInfos, const TString& url, ELanguage lang)
        : Impl(new TImpl(cfg, docInfos, url, lang))
    {
    }

    TSchemaOrgArchiveViewer::~TSchemaOrgArchiveViewer() {
    }

    IArchiveViewer& TSchemaOrgArchiveViewer::GetViewer() {
        return *Impl.Get();
    }

    const TOffer* TSchemaOrgArchiveViewer::GetOffer() const {
        return Impl->Offer.Get();
    }

    const TMovie* TSchemaOrgArchiveViewer::GetMovie() const {
        return Impl->Movie.Get();
    }

    const TRating* TSchemaOrgArchiveViewer::GetRating() const {
        return Impl->Rating.Get();
    }

    const TSozlukComments* TSchemaOrgArchiveViewer::GetEksisozlukComments() const {
        return Impl->EksisozlukComments.Get();
    }

    const TYoutubeChannel* TSchemaOrgArchiveViewer::GetYoutubeChannel() const {
        return Impl->YoutubeChannel.Get();
    }

    const TCreativeWork* TSchemaOrgArchiveViewer::GetCreativeWork() const {
        return Impl->CreativeWork.Get();
    }

    const TQuestion* TSchemaOrgArchiveViewer::GetQuestion() const {
        return Impl->Question.Get();
    }

    const TSoftwareApplication* TSchemaOrgArchiveViewer::GetSoftwareApplication() const {
        return Impl->SoftwareApplication.Get();
    }
}
