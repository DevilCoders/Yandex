#include "progress_callback_factory.h"

#include <utility>

#include "null_progress_callback.h"
#include "stream_progress_callback.h"

namespace NDoom {


template<class Base>
class TFactoryProgressCallback : public TAbstractProgressCallback {
public:
    template<class... Args>
    TFactoryProgressCallback(Args&&... args) : Base_(std::forward<Args>(args)...) {}

    void Restart(const TString& name) override {
        Base_.Restart(name);
    }

    void Step(ui64 current, ui64 total) override {
        Base_.Step(current, total);
    }

    void Step() override {
        Base_.Step();
    }

private:
    Base Base_;
};


TAbstractProgressCallback* NewNullProgressCallback() {
    return new TFactoryProgressCallback<TNullProgressCallback>();
}

TAbstractProgressCallback* NewStreamProgressCallback(IOutputStream* stream) {
    return new TFactoryProgressCallback<TStreamProgressCallback>(stream);
}


} // namespace NDoom
