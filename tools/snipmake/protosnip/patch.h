#pragma once

namespace NMetaProtocol {
    class TDocument;
}

namespace NSnippets {
    class TPassageReply;

    void PatchByPassageReply(NMetaProtocol::TDocument& doc, const TPassageReply& res);

}
