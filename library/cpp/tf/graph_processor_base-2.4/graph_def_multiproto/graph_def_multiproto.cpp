#include "graph_def_multiproto.h"

#include <netinet/in.h>
#include <util/generic/size_literals.h>

using namespace NFTMoon;

class TLimitedStream : public IInputStream {
public:
    TLimitedStream(IInputStream& input, size_t size)
    : Input(input)
    , Left(size)
    {}

    ~TLimitedStream() {
        Y_ENSURE(Left == 0);
    }

protected:
    size_t DoRead(void* buf, size_t len) final {
        if (len > Left) {
            len = Left;
        }
        if (len > 0) {
            auto read = Input.Read(buf, len);
            Left -= read;
            return read;
        } else {
            return 0;
        }
    }

private:
    IInputStream& Input;
    size_t Left = 0;
};

tensorflow::GraphDef TGraphDefMultiprotoAdapter::GetProto() {
    tensorflow::GraphDef graph;
    if (Multiproto) {
        ui32 nodesNum = 0;
        ProtoInput.LoadOrFail(&nodesNum, sizeof(nodesNum));
        nodesNum = ntohl(nodesNum);
        for (ui32 i = 0; i < nodesNum; ++i) {
            ui32 size = 0;
            ProtoInput.LoadOrFail(&size, sizeof(size));
            size = ntohl(size);
            tensorflow::NodeDef node;
            TLimitedStream nodeStream(ProtoInput, size);
            Y_ENSURE(node.ParseFromArcadiaStream(&nodeStream), "Can't parse node from stream as binary proto");
            *graph.add_node() = std::move(node);
        }
    } else {
        Y_ENSURE(graph.ParseFromArcadiaStream(this), "Can't parse stream as binary proto");
    }
    return graph;
}

TString NFTMoon::MultiProtoHeader(ui64 numNodes) {
    TStringStream stream("\x7f");
    stream << TString(TGraphDefMultiprotoAdapter::VersionString);
    Y_ENSURE(numNodes < Max<ui32>());
    ui32 size = htonl((ui32)numNodes);
    stream.Write(&size, sizeof(size));
    return stream.Str();
}

TString NFTMoon::MultiProtoPart(TString node) {
    TStringStream stream;
    Y_ENSURE(node.Size() < Max<ui32>());
    ui32 size = htonl((ui32)node.Size());
    stream.Write(&size, sizeof(size));
    stream << node;
    return stream.Str();
}

TString NFTMoon::ToMultiProto(const tensorflow::GraphDef& graph) {
    TStringStream stream(MultiProtoHeader(graph.node().size()));
    for (const auto& node : graph.node()) {
        TStringStream nodeStream;
        node.SerializeToArcadiaStream(&nodeStream);
        stream << MultiProtoPart(nodeStream.Str());
    }
    Y_ENSURE(!graph.has_versions());
    Y_ENSURE(graph.version() == 0);
    Y_ENSURE(!graph.has_library());
    return stream.Str();
}
