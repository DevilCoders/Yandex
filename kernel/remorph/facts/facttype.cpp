#include "facttype.h"

#include <kernel/remorph/facts/factmeta.pb.h>

#include <kernel/remorph/common/verbose.h>
#include <kernel/remorph/misc/proto_parser/get_option.h>

#include <util/folder/dirut.h>
#include <util/folder/path.h>
#include <util/generic/algorithm.h>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/memory/tempbuf.h>
#include <util/string/split.h>
#include <util/string/vector.h>

#include <google/protobuf/compiler/importer.h>

namespace NFact {

static const TUtf16String WSPACE = u" ";
static const TString STR_COMMA = ",";

#define GET_TEXT_CASE_FLAG NProtoParser::CreateOptionGetter(text_case)
#define GET_NORM_FLAG NProtoParser::CreateOptionGetter(norm)
#define IS_PRIME NProtoParser::CreateOptionGetter(prime)
#define GET_HEAD NProtoParser::CreateOptionGetter(head)

#define GET_FILTER NProtoParser::CreateOptionGetter(filter)
#define GET_DOMINANTS NProtoParser::CreateOptionGetter(dominants)
#define GET_SEARCH_TYPE NProtoParser::CreateOptionGetter(search_type)
#define GET_MATCHER NProtoParser::CreateOptionGetter(matcher)
#define GET_MATCHER_TYPE NProtoParser::CreateOptionGetter(matcher_type)
#define GET_GAZETTEER NProtoParser::CreateOptionGetter(gazetteer)
#define IS_AMBIG_GAZETTEER NProtoParser::CreateOptionGetter(ambig_gazetteer)
#define IS_AMBIG_CASCADE NProtoParser::CreateOptionGetter(ambig_cascade)
#define GET_GAZETTEER_RANK_METHOD NProtoParser::CreateOptionGetter(gazetteer_rank_method)
#define GET_CASCADE_RANK_METHOD NProtoParser::CreateOptionGetter(cascade_rank_method)

static TString ResolveAbsolutePath(const TString& relativeTo, const TString& path) {
    if (!path.empty()) {
        TTempBuf buf;
        ResolvePath(path.data(), TFsPath(relativeTo).Parent().RealPath().c_str(), buf.Data(), false);
        return buf.Data();
    }
    return path;
}

namespace NPrivate {

void THeadCalculator::Update(const TFieldType* field, const NRemorph::TSubmatch& range) {
    // Use head field with smallest index (it appears earlier in message definition)
    if (field->IsHead()
        && (HeadField == nullptr || HeadField->GetProtoField().index() > field->GetProtoField().index())) {

        // If we chose another head field than reset head position to new location.
        // Otherwise, for repeated values of the same field, use the earlier position
        // in case of positive offset, or later position in case of negative offset.
        if (field != HeadField
            || (HeadField->GetHeadOffset() >= 0 && HeadRange.second > range.second)
            || (HeadField->GetHeadOffset() < 0 && HeadRange.second < range.second)) {

            HeadRange = range;
        }

        // Remember head field
        HeadField = field;
    }
}

size_t THeadCalculator::GetHeadPos(const TInputSymbols& symbols) const {
    const size_t headPos = GetSymbolOffset();
    Y_ASSERT(headPos < symbols.size());
    const TInputSymbol* headSymbol = symbols[headPos].Get();
    while (headSymbol->GetHead() != nullptr) {
        headSymbol = headSymbol->GetHead();
    }
    return headSymbol->GetSourcePos().first;
}

} // NPrivate

void TFieldTypeContainer::Init(const Descriptor& desc) {
    for (int i = 0; i < desc.field_count(); ++i) {
        const FieldDescriptor* f = desc.field(i);
        switch (f->type()) {
        case FieldDescriptor::TYPE_BOOL:
            if (f->is_repeated())
                throw yexception() << "The field '" << f->name() << "' has unsupported 'repeated' flag";
            break;
        case FieldDescriptor::TYPE_STRING:
        case FieldDescriptor::TYPE_MESSAGE:
            break;
        default:
            throw yexception() << "The field '" << f->name() << "' has unsupported value type";
        }
        // Remember mapping using original field name
        NameToField[f->name()] = Fields.size();
        // If field specifies alternative name ('origin' option) then set mapping for it too.
        if (f->options().HasExtension(origin)) {
            NameToField[f->options().GetExtension(origin)] = Fields.size();
        }

        Fields.push_back(new TFieldType(*f));
        if (Fields.back()->IsCompound()) {
            Fields.back()->Init(*f->message_type());
        }
    }
}

TFieldType::TFieldType(const FieldDescriptor& f)
    : Name(f.name())
    , ProtoField(&f)
    , Prime(IS_PRIME(f))
    , Norm(ENormalization(GET_NORM_FLAG(f)))
    , TextCase(ETextCase(GET_TEXT_CASE_FLAG(f)))
    , Head(f.options().HasExtension(head))
    , HeadOffset(Head ? GET_HEAD(f) : 0)
{
}

TFactType::TFactType(const Descriptor& desc)
    : Descript(&desc)
{
    Init(desc);
}

TFactType::TFactType(const Descriptor& desc, const TString& descrPath)
    : Descript(&desc)
{
    // Read and setup gazetteer
    GazetteerPath = ResolveAbsolutePath(descrPath, GET_GAZETTEER(desc));
    REPORT(INFO, "\tGazetteer path: " << GazetteerPath);

    // Read and setup remorph
    MatcherPath = ResolveAbsolutePath(descrPath, GET_MATCHER(desc));
    if (MatcherPath.empty()) {
        throw yexception() << "Fact type definition doesn't specify the file with matcher rules";
    }
    REPORT(INFO, "\tMatcher path: " << MatcherPath);
    Init(desc);
}

void TFactType::Init(const Descriptor& desc) {
    REPORT(INFO, "Initializing '" << GetTypeName() << "' fact type");
    TFieldTypeContainer::Init(desc);
    MatcherType = NMatcher::EMatcherType(GET_MATCHER_TYPE(desc));
    switch (MatcherType) {
    case NMatcher::MT_REMORPH:
        REPORT(INFO, "\tMatcher type: REMORPH");
        break;
    case NMatcher::MT_TOKENLOGIC:
        REPORT(INFO, "\tMatcher type: TOKENLOGIC");
        break;
    case NMatcher::MT_CHAR:
        REPORT(INFO, "\tMatcher type: CHAR");
        break;
    default:
        ythrow yexception() << "Fact type definition specifies unsupported matcher type";
    }

    AmbigGazetteer = IS_AMBIG_GAZETTEER(desc);
    TStringBuf gazetteerRankMethod = GET_GAZETTEER_RANK_METHOD(desc);
    if (!gazetteerRankMethod.empty()) {
        GazetteerRankMethod = NSolveAmbig::TRankMethod(gazetteerRankMethod);
    } else {
        GazetteerRankMethod = NSolveAmbig::DefaultRankMethod();
    }
    REPORT(INFO, "\tGazetteer rank method: " << GazetteerRankMethod);

    AmbigCascade = IS_AMBIG_CASCADE(desc);
    TStringBuf cascadeRankMethod = GET_CASCADE_RANK_METHOD(desc);
    if (!cascadeRankMethod.empty()) {
        CascadeRankMethod = NSolveAmbig::TRankMethod(cascadeRankMethod);
    } else {
        CascadeRankMethod = NSolveAmbig::DefaultRankMethod();
    }
    REPORT(INFO, "\tCascade rank method: " << CascadeRankMethod);

    // Read and setup filters
    Filter = GET_FILTER(desc);
    if (!Filter.empty()) {
        REPORT(INFO, "\tFilter: " << Filter);
    }

    // Read and setup dominants
    TString dominantVal = GET_DOMINANTS(desc);
    if (!dominantVal.empty()) {
        TVector<TString> values;
        StringSplitter(dominantVal).SplitBySet(",; ").AddTo(&Dominants);
        REPORT(INFO, "\tDominants: " << JoinStrings(Dominants.begin(), Dominants.end(), STR_COMMA));
    }

    SearchMethod = NMatcher::ESearchMethod(GET_SEARCH_TYPE(desc));
}

void TFactType::JoinRanges(TVector<NRemorph::TSubmatch>& coveredRanges) {
    if (coveredRanges.size() > 1) {
        ::StableSort(coveredRanges.begin(), coveredRanges.end());
        TVector<NRemorph::TSubmatch>::iterator iCurrentRange = coveredRanges.begin();
        TVector<NRemorph::TSubmatch>::iterator i = coveredRanges.begin() + 1;
        while (i != coveredRanges.end()) {
            if (i->first <= iCurrentRange->second) {
                if (i->second > iCurrentRange->second) {
                    iCurrentRange->second = i->second;
                }
                i = coveredRanges.erase(i);
            } else {
                iCurrentRange = i++;
            }
        }
    }
}

} // NFact
