#pragma once

#include <yweb/robot/erf/mosaic/collect/collect.h>
#include <robot/library/oxygen/indexer/object_context/object_context.h>

template <class T>
bool PatchFromTuples(NOxygen::TObjectContext objectContext, const NOxygen::TTupleNameSet& tupleNames, T *target) {
    TBuffer buffer;
    for (auto tupleName : tupleNames) {
        if (objectContext.Has(tupleName)) {
            TStringBuf tupleAsString = objectContext.Get<TStringBuf>(tupleName);
            if (tupleAsString.size() && !NErfMosaic::ApplyRecordPatch<T>(buffer, tupleAsString, NErfMosaic::TPatchContext::For<T>())) {
                return false;
            }
        }
    }

    // For backward compatibility, in case we are getting records of bigger size - truncate to sizeof(T)
    size_t copySize = Min<size_t>(sizeof(T), buffer.size());
    memcpy(target, buffer.data(), copySize);
    return true;
}

template <class T>
bool CopyFromTuple(NOxygen::TObjectContext objectContext, const TString& tupleName, T *target) {
    if (objectContext.Has(tupleName)) {
        TStringBuf tupleAsString = objectContext.Get<TStringBuf>(tupleName);
        size_t recordSize = tupleAsString.size();
        if (sizeof(T) < recordSize) {
            TStringBuf url;
            if (objectContext.Has("URL"))
                url = objectContext.Get<TStringBuf>("URL");
            ythrow yexception() << "Tuple " << tupleName << ": buffer is too big (" << recordSize << ") to apply to target class (" << sizeof(T) << "), url: " << url <<  Endl;
        }
        std::char_traits<char>::assign((char *)target + recordSize, sizeof(T) - recordSize, '\0');

        // For backward compatibility, in case we are getting records of bigger size - truncate to sizeof(T)
        size_t copySize = Min<size_t>(sizeof(T), tupleAsString.size());
        memcpy(target, tupleAsString.data(), copySize);
        return true;
    }
    return false;
}

template <class T>
bool TryApplyMosaic(const TStringBuf& data, T& erf) {
    NErfMosaic::TRecordPatch patch;
    if (!patch.ParseFromArray(data.data(), data.size())) {
        return false;
    }
    if (patch.GetRecordSignature() != NErf::TStaticReflection<T>::SIGNATURE) {
        return false;
    }

    TBuffer buffer(reinterpret_cast<char*>(&erf), sizeof(T));
    NErfMosaic::ApplyRecordPatch<T>(buffer, patch, NErfMosaic::TPatchContext::For<T>());
    MemCopy(reinterpret_cast<char*>(&erf), buffer.Data(), buffer.Size());

    return true;
}

template <class T>
bool TryApplyRaw(const TStringBuf& data, T& erf) {
    if (data.size() > sizeof(T)) {
        return false;
    }

    Zero(erf);
    MemCopy(reinterpret_cast<char*>(&erf), data.data(), data.size());
    return true;
}
