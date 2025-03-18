#include "detect.h"
#include "mset.h"
#include "writer.h"
#include <util/generic/vector.h>
#include <util/generic/buffer.h>
#include <util/stream/buffer.h>
#include <util/generic/noncopyable.h>

namespace NHtmlDetect {
    class TDetector::TImpl : TNonCopyable {
    public:
        TImpl()
            : DocStream(Document)
            , Writer(&DocStream)
        {
        }
        void OnEvent(const THtmlChunk& e);
        const TDetectResult& GetResult() {
            return Machine.GetResult();
        }

    private:
        TBuffer Document;
        TBufferOutput DocStream;
        TWriter Writer;
        TMachineSet Machine;
    };

    TDetector::TDetector()
        : Impl(new TImpl)
    {
    }
    TDetector::~TDetector() {
    }
    void TDetector::Reset() {
        Impl.Reset(new TImpl);
    }
    void TDetector::OnEvent(const THtmlChunk& e) {
        Impl->OnEvent(e);
    }
    const TDetectResult& TDetector::GetResult() {
        return Impl->GetResult();
    }

    void TDetector::TImpl::OnEvent(const THtmlChunk& e) {
        // Clear the buffer here to save memory and work in stream mode
        // code is known to work without this Clear() and accumulate entire document
        // annotated with writer marks. Later finalization expressions can be run on
        // such documents and provide support for e.g. xpath axes and [indices]
        Document.Clear();

        size_t start = Document.size();
        HTLEX_TYPE etype = Writer.WriteEvent(e);
        size_t written = Document.size() - start;
        if (!written)
            return;

        if (etype == HTLEX_START_TAG) { // push state
            Machine.PushState();
        }

        Machine.PushText(Document.data() + start, written);

        if (etype == HTLEX_END_TAG) { // pop state
            Machine.PopState();
        }
    }

}
