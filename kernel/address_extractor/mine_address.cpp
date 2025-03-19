#include "mine_address.h"

#include <kernel/geograph/geograph.h>

#include <util/stream/output.h>
#include <util/stream/input.h>
#include <util/charset/unidata.h>

static const TString GEO_ADDR =              TString("geo_addr");
static const TString ANSWER =                TString("answer");

static const TString GEO =                   TString("geo");

static const TString COUNTRY =               TString("country");
const TString CITY =                  TString("city");
static const TString CITY_DESCRIPTION =      TString("g_descr");
static const TString DISCRICT1 =             TString("district1");
static const TString DISCRICT1_DESCRPTION =  TString("d_descr1");
static const TString DISCRICT2 =             TString("district2");
static const TString DISCRICT2_DESCRPTION =  TString("d_descr2");
static const TString VILLAGE =               TString("vill");
static const TString VILLAGE_DESCRIPTION =   TString("der_descr");

static const TString ADDR =                  TString("addr");

static const TString METRO =                 TString("metro");
static const TString METRO_DESCRPTION =      TString("m_descr");
static const TString QUATER =                TString("quarter");
static const TString QUATER_DESCRPTION =     TString("q_descr");
static const TString STREET =                TString("street");
static const TString STREET_DESCRPTION =     TString("descr");

static const TString NUMBERS =               TString("numbers");

static const TString GEO_ADDR_FIELDS[] = { COUNTRY, CITY_DESCRIPTION, CITY, DISCRICT1_DESCRPTION, DISCRICT1
                                         , DISCRICT2_DESCRPTION, DISCRICT2, VILLAGE_DESCRIPTION, VILLAGE
                                         , METRO_DESCRPTION, METRO, QUATER_DESCRPTION, QUATER
                                         , STREET_DESCRPTION, STREET
                                        };
static const TString GEO_FIELDS[] = { COUNTRY, CITY_DESCRIPTION, CITY, DISCRICT1_DESCRPTION, DISCRICT1
                                         , DISCRICT2_DESCRPTION, DISCRICT2, VILLAGE_DESCRIPTION, VILLAGE
                                        };
static const TString ADDR_FIELDS[] = { METRO_DESCRPTION, METRO, QUATER_DESCRPTION, QUATER
                                    , STREET_DESCRPTION, STREET
                                    };

TString AGREE_FIELDS[8] = { STREET, QUATER, METRO, VILLAGE, CITY, DISCRICT2, DISCRICT1, COUNTRY};
TString DESCRPTION_FIELDS[8] = { STREET_DESCRPTION, QUATER_DESCRPTION, METRO_DESCRPTION, VILLAGE_DESCRIPTION, CITY_DESCRIPTION, DISCRICT2_DESCRPTION,
                                DISCRICT1_DESCRPTION, ""};

size_t AGREE_FIELDS_SIZE = sizeof(AGREE_FIELDS) / sizeof(TString);

size_t MIN_GEO_INDEX = 3;


struct THasGeoAncestorFunctor {
    const TUtf16String ParentName;

    THasGeoAncestorFunctor(const TUtf16String& parentName)
        : ParentName(parentName)
    {
    }

    bool operator() (const TWtringBuf& title) {
        return title == ParentName;
    }
};

bool HasGeoAncestor(const NGzt::TArticlePtr& childArticle, const NGzt::TArticlePtr& parentArticle) {
    THasGeoAncestorFunctor f(ToWtring(NGztSupport::GetLanguageIndependentTitle(parentArticle.GetTitle())));
    return NGeoGraph::TraverseGeoPartsFirst(childArticle, f);
}


//smaller means our address is subset of param address (here all fields but numbers and descriptors)
bool TMineAddress::IsSmaller(TVector<NGzt::TArticlePtr> self, TVector<NGzt::TArticlePtr> other) {
    for (TVector<NGzt::TArticlePtr>::const_iterator it = self.begin(); self.end() != it; it++) {

        if (!it->IsInstance(it->GetArticlePool()->ProtoPool().FindMessageTypeByName("TGeoArticle"))) {
            continue;
        }

        if (std::find(other.begin(), other.end(), *it) == other.end())
            return false;
    }

    return true;
}


static TUtf16String RemoveSpaceAndPunct(const TUtf16String& str) {
    TUtf16String ret;
    for(size_t i = 0; i < str.length(); i++) {
        if (IsWhitespace(str[i]))
                continue;
        if (IsPunct(str[i]))
                continue;
        ret.push_back(str[i]);
    }
    return ret;
}


//smaller means our address is subset of param address
bool TMineAddress::IsSmaller(TMineAddressPtr fact) const {

    for(size_t level = 0; level < AGREE_FIELDS_SIZE; level++) {
        if (!IsSmaller(GetFieldArticles(AGREE_FIELDS[level]), fact->GetFieldArticles(AGREE_FIELDS[level])))
                return false;
    }

    if (RemoveSpaceAndPunct(Numbers) != RemoveSpaceAndPunct(fact->Numbers))
        return false;

    return true;
}


bool TMineAddress::IsAgreedWithGeoPart(TMineAddressPtr geo) const {

    TVector< TVector<NGzt::TArticlePtr> > selfArticles;
    for(size_t level = 0; level < AGREE_FIELDS_SIZE; level++) {
        selfArticles.push_back(GetFieldArticles(AGREE_FIELDS[level]));
    }
    TCombinationBuilder<NGzt::TArticlePtr> selfCombinationBuilder(selfArticles);

    TVector< TVector<NGzt::TArticlePtr> > geoArticles;
    for(size_t level = 0; level < AGREE_FIELDS_SIZE; level++) {
        geoArticles.push_back(geo->GetFieldArticles(AGREE_FIELDS[level]));
    }
    TCombinationBuilder<NGzt::TArticlePtr> geoCombinationBuilder(geoArticles);

    do {
        const TVector< NGzt::TArticlePtr >&  selfCombination = selfCombinationBuilder.GetCombination();

        do {
            const TVector< NGzt::TArticlePtr >&  geoCombination = geoCombinationBuilder.GetCombination();

            bool agreed = true;
            for(size_t i = 0; i < AGREE_FIELDS_SIZE; i++) {
                if (selfCombination[i].IsNull())
                    continue;

                for(size_t j = MIN_GEO_INDEX; j < AGREE_FIELDS_SIZE; j++) {
                    if (geoCombination[j].IsNull())
                        continue;

                    if ((i < j) && !HasGeoAncestor(selfCombination[i], geoCombination[j])) {
                        agreed = false;
                        break;
                    }
                    if ((i > j) && !HasGeoAncestor(geoCombination[j], selfCombination[i])) {
                        agreed = false;
                        break;
                    }
                    if ((i == j) && !(selfCombination[i] == geoCombination[j])) {
                        agreed = false;
                        break;
                    }

                }
                if (!agreed)
                    break;
            }

            if (agreed)
                return true;

            if (!geoCombinationBuilder.Shift())
                break;

        } while(true);


        if (!selfCombinationBuilder.Shift())
            break;

    } while(true);

    return false;
}


TVector<TMineAddressPtr> TMineAddress::CombineWithGeoPart(TMineAddressPtr geo) const {
    TVector<TMineAddressPtr> result;
    TVector<TString> agreededFields;

    size_t maxAddrIndex = GetMaxFilledIndex();

    if (maxAddrIndex >= geo->GetMaxFilledIndex())
        return result;

    TVector< TVector<NGzt::TArticlePtr> > combinationArticles;
    TVector< NGzt::TArticlePtr > descriptionActicles;
    TVector< TUtf16String > combinationOriginalTexts;
    TVector< TUtf16String > descriptionOriginalTexts;
    for(size_t i = 0; i < AGREE_FIELDS_SIZE; i++) {
        descriptionActicles.push_back(NGzt::TArticlePtr());
        if (i <= maxAddrIndex || HasField(AGREE_FIELDS[i])) {
            combinationArticles.push_back(GetFieldArticles(AGREE_FIELDS[i]));
            if (HasField(AGREE_FIELDS[i]))
                combinationOriginalTexts.push_back(GetFieldOriginalText(AGREE_FIELDS[i]));
            if (HasField(DESCRPTION_FIELDS[i])) {
                descriptionActicles[i] = GetFieldArticles(DESCRPTION_FIELDS[i])[0];
                descriptionOriginalTexts.push_back(GetFieldOriginalText(DESCRPTION_FIELDS[i]));
            }
        } else {
            combinationArticles.push_back(geo->GetFieldArticles(AGREE_FIELDS[i]));
            if (geo->HasField(AGREE_FIELDS[i]))
                combinationOriginalTexts.push_back(geo->GetFieldOriginalText(AGREE_FIELDS[i]));
            if (geo->GetFieldArticles(AGREE_FIELDS[i]).size() > 0) {
                agreededFields.push_back(AGREE_FIELDS[i]);
                if (geo->HasField(DESCRPTION_FIELDS[i])) {
                    descriptionActicles[i] = geo->GetFieldArticles(DESCRPTION_FIELDS[i])[0];
                    descriptionOriginalTexts.push_back(geo->GetFieldOriginalText(DESCRPTION_FIELDS[i]));
                }
            }
        }
    }

    TCombinationBuilder<NGzt::TArticlePtr> combinationBuilder(combinationArticles);

    do {
        const TVector< NGzt::TArticlePtr >&  combination = combinationBuilder.GetCombination();

        bool agreed = true;
        for(size_t i = 0; i <= maxAddrIndex; i++) {
            if (combination[i].IsNull())
                continue;
            for(size_t j = maxAddrIndex + 1; j < AGREE_FIELDS_SIZE; j++) {
                if (combination[j].IsNull())
                    continue;
                if (!HasGeoAncestor(combination[i], combination[j])) {
                    agreed = false;
                    break;
                }
            }
            if (!agreed)
                break;
        }

        if (agreed) {
            result.push_back(TMineAddressPtr(new TMineAddress(Fact,
                                                              OriginalPositions,
                                                              combination,
                                                              agreededFields,
                                                              descriptionActicles,
                                                              combinationOriginalTexts,
                                                              descriptionOriginalTexts)));
        }

        if (!combinationBuilder.Shift())
            break;

    } while(true);

    return result;
}


TMineAddress::TMineAddress(const NFact::TFactPtr& addrFact, const std::pair<size_t, size_t>& originalPositions, const TVector< NGzt::TArticlePtr>& combination,
                           const TVector<TString>& aggreedFields, const TVector<NGzt::TArticlePtr>& descriptionActicles,
                           const TVector<TUtf16String>& combinationOriginalTexts, TVector<TUtf16String>& descriptionOriginalTexts)
                : AgreededFields(aggreedFields)
                , Fact(addrFact)
                , OriginalPositions(originalPositions)
{
    size_t cIndex = 0;
    size_t dIndex = 0;
    for(size_t i = 0; i < AGREE_FIELDS_SIZE; i++) {
        if (!combination[i].IsNull()) {
            Articles.insert(std::make_pair(AGREE_FIELDS[i], TVector<NGzt::TArticlePtr>(1, combination[i])));
            FieldOriginalTexts.insert(std::make_pair(AGREE_FIELDS[i], combinationOriginalTexts[cIndex++]));
        }
        if (!descriptionActicles[i].IsNull()) {
            Articles.insert(std::make_pair(DESCRPTION_FIELDS[i], TVector<NGzt::TArticlePtr>(1, descriptionActicles[i])));
            FieldOriginalTexts.insert(std::make_pair(DESCRPTION_FIELDS[i], descriptionOriginalTexts[dIndex++]));
        }
    }

    NFact::TFieldValuePtr field;
    if (GetSubField(addrFact, ADDR, NUMBERS, &field)) {
        Numbers = field->GetText();
    }

    for(size_t i = 0; i < addrFact->GetSymbols().size(); i++) {
        OriginalText << addrFact->GetSymbols()[i]->GetText() << " ";
    }
}


TMineAddress::TMineAddress(const NFact::TFactPtr& addrFact, const std::pair<size_t, size_t>& originalPositions)
                : Fact(addrFact)
                , OriginalPositions(originalPositions)
{
    for(size_t i = 0; i < sizeof(GEO_FIELDS)/sizeof(TString); i++) {
        NFact::TFieldValuePtr field;
        if (GetSubField(addrFact, GEO, GEO_FIELDS[i], &field)) {
            Articles.insert(std::make_pair(GEO_FIELDS[i], field->GetArticles()));
            FieldOriginalTexts.insert(std::make_pair(GEO_FIELDS[i], field->GetText()));
        }
    }
    for(size_t i = 0; i < sizeof(ADDR_FIELDS)/sizeof(TString); i++) {
        NFact::TFieldValuePtr field;
        if (GetSubField(addrFact, ADDR, ADDR_FIELDS[i], &field)) {
            Articles.insert(std::make_pair(ADDR_FIELDS[i], field->GetArticles()));
            FieldOriginalTexts.insert(std::make_pair(ADDR_FIELDS[i], field->GetText()));
        }
    }
    NFact::TFieldValuePtr field;
    if (GetSubField(addrFact, ADDR, NUMBERS, &field)) {
        Numbers = field->GetText();
    }

    for(size_t i = 0; i < addrFact->GetSymbols().size(); i++) {
        OriginalText << addrFact->GetSymbols()[i]->GetText() << " ";
    }
}


TString TMineAddress::FirstVariantAsStroka() const {
    TStringStream stream;

    for(size_t i = 0; i < sizeof(GEO_ADDR_FIELDS)/sizeof(TString); i++) {
        if (HasField(GEO_ADDR_FIELDS[i])) {
            stream << GetFirstFieldValue(GEO_ADDR_FIELDS[i]) << " ";
        }
    }
    stream << Numbers;

    return stream.Str();
}


TString TMineAddress::FirstVariantAsFullAnswer() const {
    TStringStream stream;

    stream << ANSWER << "[" << FirstVariantAsStroka() << "]\t";
    stream << GEO_ADDR << "[" << OriginalText.Str() << "]\t";

    for(size_t i = 0; i < sizeof(GEO_FIELDS)/sizeof(TString); i++) {
        if (HasField(GEO_FIELDS[i])) {
            stream << GEO << "." << GEO_FIELDS[i] << "[" << GetFirstFieldValue(GEO_FIELDS[i]) << "]\t";
        }
    }
    for(size_t i = 0; i < sizeof(ADDR_FIELDS)/sizeof(TString); i++) {
        if (HasField(ADDR_FIELDS[i])) {
            stream << ADDR << "." << ADDR_FIELDS[i] << "[" << GetFirstFieldValue(ADDR_FIELDS[i]) << "]\t";
        }
    }
    if (AgreededFields.size() > 0) {
        stream << "added[ ";
        for(size_t i = 0; i < AgreededFields.size(); i++) {
            stream << AgreededFields[i] << " ";
        }
        stream << "]\t";
    }

    if (HasNumbers()) {
        stream << ADDR << "." << NUMBERS << "[" << Numbers << "]\t";
    }

    return stream.Str();
}


bool TMineAddress::GetSubField(const NFact::TFactPtr& geoFact, const TString& fieldName, const TString& subFieldName,
                 NFact::TFieldValuePtr* retField) const {
    TVector<NFact::TCompoundFieldValuePtr> geoValues = geoFact->GetCompoundValues(fieldName);

    if (geoValues.size() == 1) {
        TVector<NFact::TFieldValuePtr> cityValues = geoValues[0]->GetValues(subFieldName);
        if (cityValues.size() == 1) {
            if (retField)
                *retField = cityValues[0];
            return true;
        }
    }
    return false;
}


bool TMineAddress::HasStreetNumber() const {
    return HasNumbers() && HasField(STREET);
}


size_t TMineAddress::GetMaxFilledIndex() const {
    size_t maxAddrIndex = MIN_GEO_INDEX - 1;
    for(size_t i = MIN_GEO_INDEX; i < AGREE_FIELDS_SIZE; i++) {
        if (HasField(AGREE_FIELDS[i]))
            maxAddrIndex = i;
    }
    return maxAddrIndex;
}


size_t TMineAddress::GetMinFilledIndex() const {
    size_t maxAddrIndex = AGREE_FIELDS_SIZE - 1;
    for(int i = AGREE_FIELDS_SIZE - 1; i >= 0; i--) {
        if (HasField(AGREE_FIELDS[i]))
            maxAddrIndex = i;
    }
    return maxAddrIndex;
}
