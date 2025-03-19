#include "manip.h"

#include <kernel/snippets/idl/snippets.pb.h>

namespace NSnippets {

int TArcManip::GetUnpAnyText(TArc& arc, IArchiveConsumer& onArchive) {
    try {
        if (!arc.IsValid()) {
            return -1;
        }
        if (!arc.GetData().Empty()) {
            return onArchive.UnpText(arc.GetData().AsUnsignedCharPtr());
        }
    } catch (...) {
        return -1;
    }
    return 0;
}
int TArcManip::GetUnpText(IArchiveConsumer& onArchive, bool byLink) {
    if (byLink) {
        LinkArc.PrefetchData();
        return GetUnpAnyText(LinkArc, onArchive);
    } else {
        TextArc.PrefetchData();
        return GetUnpAnyText(TextArc, onArchive);
    }
}

void TArc::SaveState(NProto::TArc& res) const {
    if (Valid.Get()) {
        res.SetValid(*Valid);
    }
    if (Data) {
        res.SetData(Data->Data(), Data->Size());
    }
    if (DescrBlob) {
        res.SetExtInfo(DescrBlob->Data(), DescrBlob->Size());
    }
}
void TArc::LoadState(const NProto::TArc& res) {
    Reset();
    if (res.HasValid() && !Valid.Get()) {
        Valid = MakeHolder<bool>(res.GetValid());
    }
    if (res.HasData() && !Data) {
        Data = MakeHolder<TBlob>(TBlob::FromString(res.GetData()));
    }
    if (res.HasExtInfo() && !DescrBlob) {
        DescrBlob = MakeHolder<TBlob>(TBlob::FromString(res.GetExtInfo()));
        ResetDescr();
    }
}
void TArc::SaveStateUnfetchedComplement(NProto::TArc& res) const {
    bool valid;
    if (!Valid.Get()) {
        valid = FetchValid();
        res.SetValid(valid);
    } else {
        valid = *Valid;
    }
    if (valid && !Data) {
        const TBlob data = FetchData();
        res.SetData(data.Data(), data.Size());
    }
    if (valid && !DescrBlob) {
        const TBlob extInfo = FetchExtInfo();
        res.SetExtInfo(extInfo.Data(), extInfo.Size());
    }
}

}
