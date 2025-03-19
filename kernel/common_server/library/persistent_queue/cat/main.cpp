#include <kernel/common_server/library/persistent_queue/abstract/config.h>
#include <kernel/common_server/library/persistent_queue/abstract/pq.h>
#include <kernel/common_server/library/persistent_queue/cat/proto/command.pb.h>
#include <library/cpp/getoptpb/getoptpb.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/yconf/patcher/unstrict_config.h>
#include <util/folder/path.h>
#include <util/stream/output.h>
#include <util/stream/file.h>

using namespace NCS;

class TWorker {
public:
    int Work(int argc, const char** argv) {
        int result = DoWork(argc, argv);
        if (!PQ->Stop()) {
            return -1;
        }
        return result;
    }
private:
    int DoWork(int argc, const char** argv) {
        TString error;
        NGetoptPb::TGetoptPbSettings settings;
        settings.ConfPathLong = "";
        settings.ConfPathShort = '\0';
        settings.ConfPathJson = "";
        settings.ConfText = "";
        settings.DumpProtoLong = "";
        settings.DumpConfig = false;
        if (!NGetoptPb::GetoptPb(argc, argv, Command, error, settings)) {
            ERROR_LOG << error << Endl;
            return -1;
        }
        if (!Init()) {
            return -1;
        }
        if (Command.HasRead()) {
            return Read();
        }
        if (Command.HasWrite()) {
            return Write();
        }
        ERROR_LOG << "Command not set" << Endl;
        return -1;
    }

    bool Init() {
        TFsPath path(Command.GetConfig());
        if (!path.Exists()) {
            ERROR_LOG << "File doesn't exists " << path.GetPath() << Endl;
            return false;
        }
        auto cfg = MakeIntrusive<TUnstrictConfig>();
        if (!cfg->Parse(path)) {
            TString errors;
            cfg->PrintErrors(errors);
            ERROR_LOG << "Errors file parse config: " << errors << Endl;
            return false;
        }
        YConfig = cfg;
        PQConfig.Init(YConfig->GetFirstChild("PQ"));
        PQ = PQConfig->Construct(TFakePQConstructionContext());
        PQ->Start();
        return true;
    }

    int Read() {
        const auto& read = Command.GetRead();
        auto timeout = FromString<TDuration>(read.GetTimeout());
        TVector<IPQMessage::TPtr> msgs;
        if (!PQ->ReadMessages(msgs, nullptr, read.GetCount(), timeout)) {
            return -1;
        }
        for (const auto& msg: msgs) {
            Cout << msg->GetMessageId() << ": " << msg->GetContent().AsStringBuf() << Endl;
            if (!read.GetNoCommit() && !PQ->AckMessage(msg)) {
                return -1;
            }
        }
        return 0;
    }

    bool WriteData(const TString& key, const TString& data) {
        auto msg = MakeAtomicShared<TPQMessageSimple>(TBlob::FromStringSingleThreaded(data), key);
        return !!PQ->WriteMessage(msg);
    }

    bool WriteFile(const TFsPath& file) {
        if (!file.Exists()) {
            ERROR_LOG << "File doesn't exist: " << file.GetPath() << Endl;
            return false;
        }
        if (!file.IsFile()) {
            ERROR_LOG << file.GetPath() << " isn't a file" << Endl;
            return false;
        }
        INFO_LOG << "Write " << file.GetPath() << "..." << Endl;
        TString key = file.GetName();
        if (Command.GetWrite().GetKey()) {
            key = Command.GetWrite().GetKey();
        }
        TFileInput fi(file);
        if (!WriteData(key, fi.ReadAll())) {
            return false;
        }
        INFO_LOG << "Write " << file.GetPath() << "...OK" << Endl;
        return true;
    }

    bool WriteDir(const TFsPath& dir) {
        if (!dir.Exists()) {
            ERROR_LOG << "Dir doesn't exist: " << dir.GetPath() << Endl;
            return false;
        }
        if (!dir.IsDirectory()) {
            ERROR_LOG << dir.GetPath() << " isn't a directory" << Endl;
            return false;
        }
        TVector<TFsPath> children;
        dir.List(children);
        for (const auto& c: children) {
            if (c.IsFile() && !WriteFile(c)) {
                return false;
            }
        }
        return true;
    }

    int Write() {
        const auto& write = Command.GetWrite();
        if (write.GetMessagesDir()) {
            if (!WriteDir(write.GetMessagesDir())) {
                return -1;
            }
        } else if (write.GetMessageFile()) {
            if (!WriteFile(write.GetMessageFile())) {
                return -1;
            }
        } else {
            for (size_t i = 0; i < write.DataSize(); ++i) {
                if (!WriteData(write.GetKey(), write.GetData(i))) {
                    return -1;
                }
            }
        }
        if (!PQ->FlushWritten()) {
            return -1;
        }
        return 0;
    }

    NPQCat::TCommand Command;
    TIntrusivePtr<TYandexConfig> YConfig;
    TPQClientConfigContainer PQConfig;
    IPQClient::TPtr PQ;
};

int main(int argc, const char** argv) {
    DoInitGlobalLog("cerr", 7, false, false);
    TWorker worker;
    return worker.Work(argc, argv);
}
