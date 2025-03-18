#include "protocol.h"


namespace NAapi {

NVcs::Vcs::Stub* CreateNewStub(const TString& proxyAddr) {
    grpc::ChannelArguments channelArgs;
    channelArgs.SetMaxReceiveMessageSize(35 * 1024 * 1024);
    std::shared_ptr<grpc::Channel> channel = grpc::CreateCustomChannel(proxyAddr.data(), grpc::InsecureChannelCredentials(), channelArgs);
    return NVcs::Vcs::NewStub(channel).release();
}

}
