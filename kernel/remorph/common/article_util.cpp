#include "article_util.h"

#include <kernel/gazetteer/common/reflectioniter.h>
#include <kernel/geograph/geograph.h>

#include <util/generic/hash_set.h>
#include <util/string/cast.h>

namespace NGztSupport {

// external
const TString MAIN_WORD = TString("mainword");
const TString GZT_COUNTRY_TYPE = TString("geo_country");

static const TString LEMMA = TString("lemma");
static const TString TEXT = TString("text");
static const TString WEIGHT = TString("weight");

TUtf16String GetLemma(const NGzt::TArticlePtr& article) {
    const NGzt::TMessage* lemmaMessage = nullptr;
    if (article.GetField<const NGzt::TMessage*, const TString&>(LEMMA, lemmaMessage)) {
        NGzt::TProtoFieldIterator<TString> it(*lemmaMessage, lemmaMessage->GetDescriptor()->FindFieldByName(TEXT));
        if (it.Ok()) {
            return UTF8ToWide(*it);
        }
    }
    return TUtf16String();
}

double GetGztWeight(const NGzt::TArticlePtr& a) {
    const NGzt::TFieldDescriptor* f = a.FindField(WEIGHT);
    if (nullptr != f) {
        switch (f->cpp_type()) {
        case NGzt::TFieldDescriptor::CPPTYPE_INT32:
            for (NGzt::TProtoFieldIterator<i32> it(*a, f); it.Ok(); ++it) {
                return double(*it);
            }
            break;
        case NGzt::TFieldDescriptor::CPPTYPE_INT64:
            for (NGzt::TProtoFieldIterator<i64> it(*a, f); it.Ok(); ++it) {
                return double(*it);
            }
            break;
        case NGzt::TFieldDescriptor::CPPTYPE_UINT32:
            for (NGzt::TProtoFieldIterator<ui32> it(*a, f); it.Ok(); ++it) {
                return double(*it);
            }
            break;
        case NGzt::TFieldDescriptor::CPPTYPE_UINT64:
            for (NGzt::TProtoFieldIterator<ui64> it(*a, f); it.Ok(); ++it) {
                return double(*it);
            }
            break;
        case NGzt::TFieldDescriptor::CPPTYPE_DOUBLE:
            for (NGzt::TProtoFieldIterator<double> it(*a, f); it.Ok(); ++it) {
                return *it;
            }
            break;
        case NGzt::TFieldDescriptor::CPPTYPE_FLOAT:
            for (NGzt::TProtoFieldIterator<float> it(*a, f); it.Ok(); ++it) {
                return double(*it);
            }
            break;
        default:
            break;
        }
    }
    return 0.0;
}

static bool HasSimpleFieldValue(const NGzt::TMessage& msg, const TString& field, const TString& value) {
    const NGzt::TFieldDescriptor* f = msg.GetDescriptor()->FindFieldByName(field);
    if (nullptr != f) {
        switch (f->cpp_type()) {
        case NGzt::TFieldDescriptor::CPPTYPE_INT32:
            for (NGzt::TProtoFieldIterator<i32> it(msg, f); it.Ok(); ++it) {
                if (ToString(*it) == value)
                    return true;
            }
            break;
        case NGzt::TFieldDescriptor::CPPTYPE_INT64:
            for (NGzt::TProtoFieldIterator<i64> it(msg, f); it.Ok(); ++it) {
                if (ToString(*it) == value)
                    return true;
            }
            break;
        case NGzt::TFieldDescriptor::CPPTYPE_UINT32:
            for (NGzt::TProtoFieldIterator<ui32> it(msg, f); it.Ok(); ++it) {
                if (ToString(*it) == value)
                    return true;
            }
            break;
        case NGzt::TFieldDescriptor::CPPTYPE_UINT64:
            for (NGzt::TProtoFieldIterator<ui64> it(msg, f); it.Ok(); ++it) {
                if (ToString(*it) == value)
                    return true;
            }
            break;
        case NGzt::TFieldDescriptor::CPPTYPE_DOUBLE:
            for (NGzt::TProtoFieldIterator<double> it(msg, f); it.Ok(); ++it) {
                if (ToString(*it) == value)
                    return true;
            }
            break;
        case NGzt::TFieldDescriptor::CPPTYPE_FLOAT:
            for (NGzt::TProtoFieldIterator<float> it(msg, f); it.Ok(); ++it) {
                if (ToString(*it) == value)
                    return true;
            }
            break;
        case NGzt::TFieldDescriptor::CPPTYPE_BOOL:
            for (NGzt::TProtoFieldIterator<bool> it(msg, f); it.Ok(); ++it) {
                if (ToString(*it) == value)
                    return true;
            }
            break;
        case NGzt::TFieldDescriptor::CPPTYPE_ENUM:
            for (NGzt::TProtoFieldIterator<const ::google::protobuf::EnumValueDescriptor*> it(msg, f); it.Ok(); ++it) {
                if ((*it)->name() == value)
                    return true;
            }
            break;
        case NGzt::TFieldDescriptor::CPPTYPE_STRING:
            for (NGzt::TProtoFieldIterator<TString> it(msg, f); it.Ok(); ++it) {
                if (*it == value)
                    return true;
            }
            break;
        default:
            // Return false for unsupported type
            break;
        }
    }
    return false;
}

bool HasFieldValue(const NGzt::TMessage& msg, const TString& field, const TString& value) {
    size_t dotPos = field.find('.');
    if (dotPos != TString::npos) {
        const NGzt::TFieldDescriptor* parent = msg.GetDescriptor()->FindFieldByName(field.substr(0, dotPos));
        if (nullptr != parent) {
            TString subField = field.substr(dotPos + 1);
            for (NGzt::TProtoFieldIterator<const NGzt::TMessage*> it(msg, parent); it.Ok(); ++it) {
                if (HasFieldValue(**it, subField, value))
                    return true;
            }
        }
        return false;
    } else {
        return HasSimpleFieldValue(msg, field, value);
    }
}

struct THasGeoAncestorFunctor {
    const THashSet<TUtf16String>& Names;
    TUtf16String* NewArticleTitle;

    THasGeoAncestorFunctor(const THashSet<TUtf16String>& names, TUtf16String* newArticleTitle = nullptr)
        : Names(names)
        , NewArticleTitle(newArticleTitle)
    {
    }

    bool operator() (const TWtringBuf& title) {
        if (Names.contains(title)) {
            if (NewArticleTitle)
                *NewArticleTitle = title;
            return true;
        }
        return false;
    }
};

bool HasGeoAncestor(const NGzt::TArticlePtr& childArticle, const THashSet<TUtf16String>& parentArticles) {
    THasGeoAncestorFunctor f(parentArticles);
    return NGeoGraph::TraverseGeoPartsFirst(childArticle, f);
}

bool HasGeoAncestor(const NGzt::TArticlePtr& childArticle, const THashSet<TUtf16String>& parentArticles, TUtf16String* newArticleTitle) {
    THasGeoAncestorFunctor f(parentArticles, newArticleTitle);
    return NGeoGraph::TraverseGeoPartsFirst(childArticle, f);
}

} // NGztSupport
