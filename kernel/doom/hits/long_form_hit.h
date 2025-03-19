#pragma once

#include <util/system/types.h>
#include <utility>


namespace NDoom {


template<class Base>
class TLongFormHit: public Base {
public:
    TLongFormHit(): Base(), Form_(0) {}

    template<class... Args>
    TLongFormHit(ui32 form, Args&&... args) : Base(std::forward<Args>(args)...), Form_(form) {}

    ui32 Form() const {
        return Form_;
    }

    void SetForm(ui32 form) {
        Form_ = form;
    }

    enum {
        FormBits = 32
    };

    friend IOutputStream& operator<<(IOutputStream& stream, const TLongFormHit& hit) {
        stream << "[" << hit.DocId() << "." << hit.Break() << "." << hit.Word() << "." << hit.Relevance() << "." << hit.Form() << "]";
        return stream;
    }
private:
    ui32 Form_;
};

template<class Hit>
TLongFormHit<Hit> MakeLongFormHit(ui32 form, const Hit& hit) {
    return TLongFormHit<Hit>(form, hit);
}


} // namespace NDoom
