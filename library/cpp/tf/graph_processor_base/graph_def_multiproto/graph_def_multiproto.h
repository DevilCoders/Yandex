#pragma once

#include <tensorflow/core/public/session.h>

/* Serializes and desirializes large proto in multiproto format BERT-436
 * Multiproto always starts with "\x7fmulti_proto_v1"
 * It is folowed by 4 bytes encoding number of nodes in net byte order
 * Each node is then written as serialized proto preceded by 4 bytes encoding lenght in net byte order
 */

namespace NFTMoon {

class TGraphDefMultiprotoAdapter : public IInputStream {
public:
    TGraphDefMultiprotoAdapter(IInputStream& protoInput)
    : ProtoInput(protoInput)
    {
        if (ProtoInput.Load(&FirstSymbol, sizeof(FirstSymbol))) {
            if (FirstSymbol == 0x7f) {
                Multiproto = true;
                char version[sizeof(VersionString) - 1] = {};
                ProtoInput.Load(version, sizeof(VersionString) - 1);
                Y_ENSURE(TString(version) == TString(VersionString));
            } else {
                Multiproto = false;
            }
        } else {
            Multiproto = false;
            StartedReading = true;
        }
    }

    bool IsMultiproto() const {
        return Multiproto;
    }

    tensorflow::GraphDef GetProto();

    static constexpr char VersionString[] = "multi_proto_v1";

protected:
    // Read regular proto
    size_t DoRead(void* buf, size_t len) final {
        Y_ENSURE(!Multiproto);
        if (!StartedReading) {
            StartedReading = true;
            Y_ENSURE(len > 0);
            *static_cast<ui8*>(buf) = FirstSymbol;
            return 1;
        } else {
            return ProtoInput.Read(buf, len);
        }
    }

private:
    IInputStream& ProtoInput;
    bool StartedReading = false;
    bool Multiproto = false;
    ui8 FirstSymbol = 0;
};

TString MultiProtoHeader(size_t numNodes);

TString MultiProtoPart(TString node);

TString ToMultiProto(const tensorflow::GraphDef& graph);

}
