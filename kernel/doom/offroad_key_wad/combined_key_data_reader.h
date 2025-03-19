#pragma once

namespace NDoom {


template <class Combiner, class Base>
class TCombinedKeyDataReader: public Base {
public:
    using TKeyRef = typename Base::TKeyRef;
    using TKeyData = typename Base::TKeyData;

    bool ReadKey(TKeyRef* key, TKeyData* data) {
        if (Combiner::IsIdentity) {
            return Base::ReadKey(key, data);
        }
        TKeyData prevData = Base::LastData();
        TKeyData nextData;
        if (!Base::ReadKey(key, &nextData)) {
            return false;
        }
        Combiner::Combine(prevData, nextData, data);
        return true;
    }
};


} // namespace NDoom
