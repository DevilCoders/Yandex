#include <tools/clustermaster/common/messages.h>
#include <tools/clustermaster/common/target_graph_impl.h>
#include <tools/clustermaster/master/master.h>
#include <tools/clustermaster/master/master_config.h>
#include <tools/clustermaster/master/master_target_graph.h>

#include <library/cpp/getopt/small/last_getopt.h>

#include <util/folder/tempdir.h>
#include <util/stream/file.h>
#include <util/system/fs.h>
#include <util/system/shellcommand.h>
#include <util/system/tempfile.h>

void WriteToFile(const TString& fileName, const TString& content) {
    TFixedBufferFileOutput outFile(fileName);
    outFile << content;
}

int main(int argc, const char* argv[]) {
    TString cmMain, hostList, outScript;
    bool enableBashCheck = true, forceDumpScript = false;

    NLastGetopt::TOpts opts;
    opts.AddLongOption('i', "input-main", "path to cm_main.sh")
        .Required()
        .RequiredArgument()
        .StoreResult(&cmMain);
    opts.AddLongOption('h', "hostlist", "path to hostlist")
        .Optional()
        .RequiredArgument()
        .DefaultValue("")
        .StoreResult(&hostList);
    opts.AddLongOption('o', "output-script", "output path to script")
        .Optional()
        .RequiredArgument()
        .StoreResult(&outScript);
    opts.AddLongOption('N', "no-bash-check", "disable `bash -n` call")
        .Optional()
        .NoArgument()
        .StoreValue(&enableBashCheck, false);
    opts.AddLongOption('f', "force-dump", "rewrite output file even if errors occurred")
        .Optional()
        .NoArgument()
        .SetFlag(&forceDumpScript);

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    TTempDir tempDir;

    if (!outScript) {
        outScript = tempDir.Name() + "/script";
    }

    TTempFile tmpOutScript(outScript + ".cmcheck.tmp");
    try {
        TAutoPtr<TConfigMessage> msg;
        TString nohost = "";
        TIpPort noport = 0;
        TMasterConfigSource configSource((TFsPath(cmMain)));
        msg = ParseMasterConfig(configSource, nohost, noport);
        WriteToFile(tmpOutScript.Name(), msg->GetConfig());

        if (hostList) {
            TMasterListManager lm;

            MasterOptions.VarDirPath = tempDir.Name();
            lm.LoadHostlist(hostList, tempDir.Name());
            TMasterGraph graph(&lm);
            graph.ParseConfig(msg.Get());
        }

        if (enableBashCheck) {
            TShellCommandOptions opts;
            opts.SetOutputStream(&Cout);
            opts.SetErrorStream(&Cerr);
            TShellCommand command("bash", {"-n", tmpOutScript.Name()}, opts);
            command.Run();
            Y_ENSURE(command.GetStatus() == TShellCommand::ECommandStatus::SHELL_FINISHED);
        }
        NFs::Rename(tmpOutScript.Name(), outScript);
    } catch (const yexception& e) {
        Cerr << e.what() << Endl;
        if (forceDumpScript) {
            NFs::Copy(tmpOutScript.Name(), outScript);
        }
        return 1;
    }
    return 0;
}
