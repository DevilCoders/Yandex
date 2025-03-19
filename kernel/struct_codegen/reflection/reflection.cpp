// Methods for working with erf records with dynamically known structure.

#include "reflection.h"

#include <util/digest/fnv.h>

#include <util/generic/hash.h>


void IErfFieldsVisitor::VisitCppField(const char* type, const char* /*name*/, const void* /*value*/, size_t /*size*/) {
    ythrow yexception() << "Unknown type " << type;
}

namespace {

void CompilerCheck() {
#ifdef __BIG_ENDIAN__
#error "Unsupported architecture"
#endif

    struct S {
        ui32 a:11;
        ui32 b:7;
        ui32 c:14;
    };
    static_assert(sizeof(S) == 4, "expect sizeof(S) == 4");
    static const S s = { 123, 45, 678 };
    ui32 u;
    memcpy(&u, &s, 4);
    Y_VERIFY(u == 123 + (45 << 11) + (678 << 18), "Bad compiler");
}

} // anonymous namespace

namespace NErf {

ui32 GetRecordSignature(const TStringBuf& name) {
    return FnvHash<ui32>(name.data(), name.size());
}

// Hides dependency on THashMap from header file
class TNameToIndexMap {
    THashMap<TStringBuf, ui32> Map;

public:
    void Add(const TStringBuf& name, ui32 index) {
        if (Map.contains(name) && Map[name] != index)
            ythrow yexception() << "Repeated name: " << name;
        Map[name] = index;
    }

    int Get(const TStringBuf& name) const noexcept {
        THashMap<TStringBuf, ui32>::const_iterator it = Map.find(name);
        return it != Map.end() ? it->second : -1;
    }

    void Clear() {
        Map.clear();
    }
};

TRecordReflection::TRecordReflection(const TStaticRecordDescriptor& descriptor)
    : RecordDescriptor(descriptor)
{
    ::CompilerCheck();

    NameToIndex.Reset(new TNameToIndexMap());

    for (size_t index = 0; index < GetFieldCount(); ++index) {
        const TStaticFieldDescriptor& field = GetFieldDescriptor(index);
        NameToIndex->Add(field.Name, index);
        if (field.PrevNames != nullptr) {
            TStringBuf prevNamesLeft(field.PrevNames);
            while (!prevNamesLeft.empty()) {
                NameToIndex->Add(prevNamesLeft.NextTok(','), index);
            }
        }
    }
}

int TRecordReflection::GetFieldIndex(const TStringBuf& name) const noexcept {
    return NameToIndex->Get(name);
}


TRecordReflection::~TRecordReflection()
{
}

const ui32 TFieldReflection::MASKS[] = {
    0,
#define FOR(bits) (((ui32)1 << bits) - 1),
    FOR(1)
    FOR(2)
    FOR(3)
    FOR(4)
    FOR(5)
    FOR(6)
    FOR(7)
    FOR(8)
    FOR(9)
    FOR(10)
    FOR(11)
    FOR(12)
    FOR(13)
    FOR(14)
    FOR(15)
    FOR(16)
    FOR(17)
    FOR(18)
    FOR(19)
    FOR(20)
    FOR(21)
    FOR(22)
    FOR(23)
    FOR(24)
    FOR(25)
    FOR(26)
    FOR(27)
    FOR(28)
    FOR(29)
    FOR(30)
    FOR(31)
#undef  FOR
    ui32(-1)
};

TVector<int> MatchFieldsByName(const TRecordReflection& fields1, const TRecordReflection& fields2) {
    int count1 = fields1.GetFieldCount();
    int count2 = fields2.GetFieldCount();
    TVector<int> res(count1, -1);

    for (int i = 0; i < count1; i++) {
        res[i] = fields2.GetFieldIndex(fields1.GetFieldName(i));
    }

    for (int j = 0; j < count2; j++) {
        int i = fields1.GetFieldIndex(fields2.GetFieldName(j));
        if (i == -1)
            continue;
        else if (res[i] == -1)
            res[i] = j;
        else if (res[i] != j)
            ythrow yexception() << "Field " << fields2.GetFieldName(j)
                << " was matched to several fields in another structure: "
                << fields1.GetFieldName(j) << " and " << fields1.GetFieldName(res[i]);
    }

    for (int i = 0; i < count1; i++) {
        if (res[i] < 0)
            continue;

        const char* name = fields1.GetFieldName(i);
        const TStaticFieldDescriptor& field1 = fields1.GetFieldDescriptor(i);
        const TStaticFieldDescriptor& field2 = fields2.GetFieldDescriptor(res[i]);

        if (field1.Type != field2.Type)
            ythrow yexception() << "Field " << name << " has different types in two structures: "
                                << (field1.Type != nullptr ? field1.Type : "bitfield") << " and "
                                << (field2.Type != nullptr ? field2.Type : "bitfield");

        if (field1.Width != field2.Width)
            ythrow yexception() << "Field " << name << " has different widths in two structures: "
                                << field1.Width << " and " << field2.Width;
    }

    return res;
}

TRecordTranslator::TRecordTranslator(const TRecordReflection& dst, const TRecordReflection& src, const TVector<int>& dst2src)
    : DstReflection(dst)
    , SrcReflection(src)
{
    Init(dst2src);
}

TRecordTranslator::TRecordTranslator(const TRecordReflection& dst, const TRecordReflection& src)
    : DstReflection(dst)
    , SrcReflection(src)
{
    TVector<int> dst2src = MatchFieldsByName(dst, src);
    Init(dst2src);
}

void TRecordTranslator::Init(const TVector<int>& dst2src) {
    const TRecordReflection& dst = DstReflection;
    const TRecordReflection& src = SrcReflection;

    for (size_t i = 0; i != dst2src.size(); ++i) {
        if (dst2src[i] < 0)
            continue;

        ui32 dstIndex = i;
        ui32 srcIndex = dst2src[i];

        if (srcIndex >= src.GetFieldCount() || dstIndex >= dst.GetFieldCount())
            ythrow yexception() << "Field index out of bounds";

        if ((src.IsBitField(srcIndex) != dst.IsBitField(dstIndex)) ||
            (src.GetFieldWidth(srcIndex) != dst.GetFieldWidth(dstIndex)))
            ythrow yexception() << "Fields are of different types or widths";
    }

    for (size_t i = 0; i != dst2src.size(); ) {
        if (dst2src[i] < 0) {
            ++i;
            continue;
        }

        const ui32 dstFieldOffset = dst.GetFieldOffset(i);
        const ui32 srcFieldOffset = src.GetFieldOffset(dst2src[i]);

        if (0 != dstFieldOffset % 8 || 0 != srcFieldOffset % 8) {
            TMove move;
            move.Dst = i;
            move.Src = dst2src[i];
            move.BlockSize = 0;
            Moves.push_back(move);
            ++i;
            continue;
        }

        // found a pair of matching fields which start at a byte boundary

        TMove block;
        block.Dst = dstFieldOffset / 8;
        block.Src = srcFieldOffset / 8;
        block.BlockSize = 0;

        // find a consecutive range [i, j) of fields which are mapped to another consecutive range
        size_t j;
        for (j = i + 1; j != dst2src.size(); ++j)
            if (0 != (dst2src[i] - dst2src[j]) + (j - i))
                break;

        // shrink it until we find a pair of fields which end at a byte boundary
        for (size_t k = j; k != i; --k) {
            const ui32 fieldEnd = dst.GetFieldOffset(k-1) + dst.GetFieldWidth(k-1);
            if (0 == fieldEnd % 8) {
                block.BlockSize = fieldEnd / 8 - block.Dst;
                if (block.BlockSize >= 32 || k - i >= 16) {
                    // big enough block
                    Moves.push_back(block);
                    i = k;
                }
                break;
            }
            // TODO: could consider inclusion of padding here, but it's full of
            // corner cases and provides little performance gain
        }

        // new range: [i, k)

        for ( ; i != j; ++i) {
            TMove move;
            move.Dst = i;
            move.Src = dst2src[i];
            move.BlockSize = 0;
            Moves.push_back(move);
        }
    }

#if 0
    fprintf(stderr, "TRecordTranslator blocks:");
    for (i = 0; i < (int)Moves.size(); i++)
        if (Moves[i].BlockSize != 0) fprintf(stderr, " %d", (int)Moves[i].BlockSize);
    fprintf(stderr, ", fields:");
    for (i = 0; i < (int)Moves.size(); i++)
        if (Moves[i].BlockSize == 0) fprintf(stderr, " %d", (int)dst.GetFieldWidth(Moves[i].Dst));
    fprintf(stderr, "\n");
#endif
}

void TRecordTranslator::Translate(void* dst, const void* src) const noexcept {
    if (Y_UNLIKELY(Moves.empty()))
        return;

    const TMove* move = &Moves[0];
    const TMove* end = move + Moves.size();

    Y_PREFETCH_READ(move, 3);
    Y_PREFETCH_READ(src, 3);
    Y_PREFETCH_WRITE(dst, 3);

    for (; move != end; ++move) {
        if (Y_LIKELY(move->BlockSize == 0))
            TranslateField(DstReflection, dst, move->Dst, SrcReflection, src, move->Src);
        else
            memcpy((char*)dst + move->Dst, (const char*)src + move->Src, move->BlockSize);
    }
}


}  // namespace NErf
