#include <library/cpp/yson_pull/yson.h>

#include <util/stream/output.h>
#include <util/generic/yexception.h>

namespace {
    NYsonPull::TWriter MakeWriter(THolder<NYsonPull::NOutput::IStream> stream, const char* type) {
        if (type != nullptr) {
            if (type[0] == 'b') {
                return NYsonPull::MakeBinaryWriter(std::move(stream), NYsonPull::EStreamType::ListFragment);
            } else if (type[0] == 't') {
                return NYsonPull::MakeTextWriter(std::move(stream), NYsonPull::EStreamType::ListFragment);
            }
        }
        return NYsonPull::MakePrettyTextWriter(std::move(stream), NYsonPull::EStreamType::ListFragment);
    }
} // anonymous namespace

int main(int argc, char* argv[]) {
    try {
        auto reader = NYsonPull::TReader(
            NYsonPull::NInput::FromPosixFd(STDIN_FILENO),
            NYsonPull::EStreamType::ListFragment);

        auto writer = MakeWriter(
            NYsonPull::NOutput::FromPosixFd(STDOUT_FILENO),
            argc > 1 ? argv[1] : nullptr);

        NYsonPull::Bridge(reader, writer);
    } catch (...) {
        Cerr << "ERROR: " << CurrentExceptionMessage() << Endl;
        return 1;
    }

    return 0;
}
