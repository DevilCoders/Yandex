#include "gztarticle.h"

#include "common/recode.h"
#include "articlepool.h"
#include <kernel/gazetteer/proto/base.pb.h>



namespace NGzt
{

TArticlePtr TArticlePtr::MakeExternalArticle(ui32 id, const TMessage& object, const TUtf16String& title) {
    Y_ASSERT(!TArticlePool::IsValidOffset(id));
    TArticlePtr res;

    res.Offset = id;
    res.Descriptor = object.GetDescriptor();
    res.Title = title;

    res.Body = TMessagePtr(object.New());
    res.Body->CopyFrom(object);

    return res;
}

bool TArticlePtr::IsInstance(const TDescriptor* type) const {
    if (IsFromPool())
        return ArticlePool->ProtoPool().IsSubType(GetType(), type);
    else
        return GetType() == type;
}

TBlob TArticlePtr::GetBinary() const
{
    Y_ASSERT(!IsNull());
    if (IsFromPool())
        return ArticlePool->GetArticleBinaryAtOffset(Offset);
    else {
        TBuffer buffer;
        buffer.Reserve(Get()->ByteSize());
        Get()->SerializeWithCachedSizesToArray(reinterpret_cast<ui8*>(buffer.Data()));
        return TBlob::FromBuffer(buffer);
    }
}

TUtf16String TArticlePtr::GetTypeName() const {
    if (Y_UNLIKELY(IsNull()))
        return TUtf16String();

    if (Y_LIKELY(IsFromPool()))
        return ArticlePool->ProtoPool().GetDescriptorName(GetType());
    else
        return TUtf16String::FromAscii(GetType()->name());        // fallback
}

void TArticlePtr::DeserializeHead()
{
    Y_ASSERT(IsFromPool());
    Descriptor = ArticlePool->FindDescriptorByOffset(Offset);
    Title = ArticlePool->FindArticleNameByOffset(Offset);
}

void TArticlePtr::DeserializeBody() const
{
    Y_ASSERT(IsFromPool());

    Body.InitOnce(ArticlePool->LoadArticleAtOffset(Offset).Release());

    Y_ASSERT(Body.Get() != nullptr);
    Y_ASSERT(Body->GetDescriptor() == Descriptor);
}

TArticlePtr TArticlePtr::LoadByOffset(ui32 offset) const {
    return (Offset != offset) ? TArticlePtr(offset, *this) : *this;
}

TArticlePtr TArticlePtr::LoadByRef(const TRef& ref) const {
    return LoadByOffset(ref.id());
}

TArticlePtr TArticlePtr::LoadByTitle(const TWtringBuf& title) const {
    TArticlePtr ret;
    if (IsFromPool())
        ArticlePool->FindArticleByName(title, ret);
    return ret;
}

void TArticlePtr::NoValueError(const TFieldDescriptor* fd) const {
    if (fd == nullptr)
        ythrow yexception() << "Cannot find specified field in article \"" << GetTitle() << "\".";
    else
        ythrow yexception() << "No value is assigned to field " << fd->name() << " (article \"" << GetTitle() << "\").";
}

bool TArticlePtr::HasNonCustomKey() const
{
    if (IsNull() || !IsFromPool())
        return false;

    TSearchKey tmpkey;
    for (NGzt::TSearchKeyIterator it(*Get(), &ArticlePool->ProtoPool(), tmpkey); it.Ok(); ++it)
        if (it->type() != TSearchKey::CUSTOM)
            return true;

    return false;
}

template <typename TNum>
static inline bool GetEnumAsNumeric(const TArticlePtr& art, const TFieldDescriptor* field, TNum& result) {
    const NProtoBuf::EnumValueDescriptor* tmp = nullptr;
    if (art.GetField(field, tmp)) {
        result = tmp->number();
        return true;
    } else
        return false;
}

#define RETURN_FIELD_AS_DOUBLE(type) {\
    type tmp;\
    if (GetField(field, tmp)) {\
        result = static_cast<double>(tmp);\
        return true;\
    } else\
        return false;\
}

bool TArticlePtr::GetNumericFieldAsDouble(const TFieldDescriptor* field, double& result) const {
    if (field == nullptr)
        return false;
    switch (field->cpp_type()) {
        case TFieldDescriptor::CPPTYPE_INT32:  RETURN_FIELD_AS_DOUBLE(i32);
        case TFieldDescriptor::CPPTYPE_UINT32: RETURN_FIELD_AS_DOUBLE(ui32);
        case TFieldDescriptor::CPPTYPE_INT64:  RETURN_FIELD_AS_DOUBLE(i64);
        case TFieldDescriptor::CPPTYPE_UINT64: RETURN_FIELD_AS_DOUBLE(ui64);
        case TFieldDescriptor::CPPTYPE_DOUBLE: RETURN_FIELD_AS_DOUBLE(double);
        case TFieldDescriptor::CPPTYPE_FLOAT:  RETURN_FIELD_AS_DOUBLE(float);
        case TFieldDescriptor::CPPTYPE_BOOL:   RETURN_FIELD_AS_DOUBLE(bool);
        case TFieldDescriptor::CPPTYPE_ENUM:   return GetEnumAsNumeric(*this, field, result);
        default:             return false;
    }
}

#undef RETURN_FIELD_AS_DOUBLE

void TRefIterator::ResetCurrent() {
    if (RefIter.Ok()) {
        // do not iterate over optional empty TRef field.
        if (RefIter.Default())
            RefIter.Reset();
        else {
            //ui32 offset = Singleton<TRefIdField>()->GetValue(**RefIter);      // TRef.Id
            const TMessage* ref = *RefIter;     // TRef
            const TFieldDescriptor* refIdField = ref->GetDescriptor()->FindFieldByNumber(TRef::kIdFieldNumber);   // TRef.id
            Offset = ref->GetReflection()->GetUInt32(*ref, refIdField);
        }
    }
}


TCustomKeyIterator::TCustomKeyIterator(const TArticlePtr& article, const TStringBuf& prefix)
    : Prefix(prefix)
    , TmpKey(new TSearchKey)
    , It(*article, &article.ArticlePool->ProtoPool(), *TmpKey)
    , KeyTextIndex(0)
{
    Y_ASSERT(article.IsFromPool());      // a little too late...
    Ok_ = It.Ok() && FindNext();
}

TCustomKeyIterator::TCustomKeyIterator(const TMessage* article, const TProtoPool& descriptors, const TString& prefix)
    : Prefix(prefix)
    , TmpKey(new TSearchKey)
    , It(*article, &descriptors, *TmpKey)
    , KeyTextIndex(0)
{
    Ok_ = It.Ok() && FindNext();
}

TCustomKeyIterator::~TCustomKeyIterator() {
    // for sake of THolder<TSearchKey>
}


bool TCustomKeyIterator::FindNext()
{
    Y_ASSERT(It.Ok());
    do {
        while (KeyTextIndex < (size_t)It->text_size()) {
            TStringBuf text = It->text(KeyTextIndex);
            ++KeyTextIndex;

            if (text.StartsWith(Prefix)) {
                text.Skip(Prefix.size());
                KeyTextWithoutPrefix.AssignNoAlias(text.data(), text.size());
                return true;
            }
        }
        ++It;
        KeyTextIndex = 0;
    } while (It.Ok());

    return false;
}




} // namespace NGzt
