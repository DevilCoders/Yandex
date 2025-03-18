#include "worker_workflow.h"

#include "log.h"
#include "master_multiplexor.h"
#include "messages.h"
#include "worker.h"
#include "worker_target_graph.h"
#include "worker_variables.h"

#include <library/cpp/deprecated/fgood/fgood.h>

#include <util/generic/singleton.h>
#include <util/stream/file.h>
#include <util/stream/format.h>
#include <util/system/fs.h>

TWorkerControlWorkflow::TWorkerControlWorkflow(TWorkerGraph& graph, THolder<TStreamSocket> socket)
    : TControlWorkflow(std::move(socket))
    , TargetGraph(graph)
    , AuthOk(false)
    , InitOk(false)
    , MasterIsSecondary(false)
    , MasterHttpPort(0)
{
}

void TWorkerControlWorkflow::OnClose() {
    DEBUGLOG("TWorkerControlWorkflow::OnClose() was called; this=" << Hex(reinterpret_cast<size_t>(this))); // added logging here while solving CLUSTERMASTER-59
    Singleton<TMasterMultiplexor>()->RemoveMaster(*this);
}

int TWorkerControlWorkflow::GetNetworkHeartbeat() const {
    return TargetGraph.WorkerGlobals->NetworkHeartbeat;
}

void TWorkerControlWorkflow::Greeting() {
    TWorkerHelloMessage message(TargetGraph.WorkerGlobals->HttpPort);
    message.GenerateResponse(ExpectedAuthReply, TargetGraph.WorkerGlobals->AuthKeyPath);
    EnqueueMessage(message);
}

void TWorkerControlWorkflow::ProcessMessage(TAutoPtr<NTransgene::TPackedMessage> what) {
    if (!AuthOk && what->GetType() != TAuthReplyMessage::Type)
        ythrow yexception() << "Authorization needed (for message type " << what->GetType() << ")";

    if (!InitOk && what->GetType() != TAuthReplyMessage::Type && what->GetType() != TConfigMessage::Type)
        ythrow yexception() << "Initialization needed (for message type " << what->GetType() << ")";

    switch (what->GetType()) {
    case TAuthReplyMessage::Type:
        {
            TAuthReplyMessage in(*what);

            LOG1(net, MasterAddr << ": Authorization request received, verifying");

            AuthOk = false;

            if (ExpectedAuthReply != in.GetDigest())
                ythrow yexception() << "Authorization failed";

            if (in.HasMasterIsSecondary() && in.GetMasterIsSecondary())
                MasterIsSecondary = true;

            AuthOk = true;

            Singleton<TMasterMultiplexor>()->AddMaster(*this);

            LOG1(net, MasterAddr << ": Authorization successed, sending notification");

            EnqueueMessage(TAuthSuccessMessage());

            // send variables
            TVariablesMessage variablesMessage;
            TargetGraph.ExportVariables(&variablesMessage);
            LOG1(net, MasterAddr << ": Sending worker variables (" << variablesMessage.FormatText() << ")");
            EnqueueMessage(variablesMessage);

            // secondary masters should not set ConfigMessage
            if (MasterIsSecondary)
                InitOk = true;
        }
        break;
    case TConfigMessage::Type:
        {
            int numPrimaryMasters = Singleton<TMasterMultiplexor>()->GetNumPrimaryMasters();
            if (MasterIsSecondary || numPrimaryMasters > 1) {
                DEBUGLOG("Got config from secondary master; MasterIsSecondary=" << MasterIsSecondary << "; GetNumPrimaryMasters()=" << numPrimaryMasters);
                ythrow yexception() << "No config allowed from secondary master";
            }

            TConfigMessage in(*what);

            LOG1(net, MasterAddr << ": Configuration received, initializing");

            InitOk = false;

            try {
                // TODO: ain't good, there should be a single place for
                // script (in shell terms) - either script or config
                //
                // Should either use script for both parsing and shell, or
                // use config for shell (i.e. keep it in memory and pipe to
                // spawned shells, or reread every time

                TString tempConfigPath = TargetGraph.WorkerGlobals->ConfigPath + ".new";
                TString tempScriptPath = TargetGraph.WorkerGlobals->ScriptPath + ".new";

                TUnbufferedFileOutput configFile(tempConfigPath);
                if (!in.SerializeToArcadiaStream(&configFile))
                    ythrow yexception() << "cannot write new config";

                TUnbufferedFileOutput scriptFile(tempScriptPath);
                try {
                    scriptFile.Write(in.GetConfig().data(), in.GetConfig().size());
                } catch (const yexception& /*e*/) {
                    ythrow yexception() << "cannot write new script";
                }

                // import it
                TargetGraph.ImportConfig(&in);

                // replace files with new ones
                if (!NFs::Rename(tempConfigPath, TargetGraph.WorkerGlobals->ConfigPath)) {
                    ythrow yexception() << "cannot replace config file: " << LastSystemErrorText();
                }
                if (!NFs::Rename(tempScriptPath, TargetGraph.WorkerGlobals->ScriptPath)) {
                    int error = LastSystemError();
                    NFs::Remove(TargetGraph.WorkerGlobals->ConfigPath);
                    NFs::Remove(TargetGraph.WorkerGlobals->ScriptPath);
                    ythrow yexception() << "cannot replace script file: " << LastSystemErrorText(error);
                }
            } catch (const yexception& e) {
                ythrow yexception() << "Initialization failed: " << e.what();
            }

            MasterHost = in.GetMasterHost();
            MasterHttpPort = in.GetMasterHttpPort();

            InitOk = true;

            // reply with full status
            { // see CLUSTERMASTER-36
                TGuard<TMutex> guard(TargetGraph.GetMutex());

                LOG1(net, MasterAddr << ": Initialized, sending full status");
                TFullStatusMessage msg;
                TargetGraph.ExportFullStatus(&msg);
                EnqueueMessage(msg);
            }

            // (variables should be saved here after applying
            // default/forced values, but that's not really necessary,
            // as on reload they will be reapplied anyway)

            // resend variables, which may have been changed in graph parsing
            LOG1(net, MasterAddr << ": Sending worker variables");
            TVariablesMessage variablesMessage;
            TargetGraph.ExportVariables(&variablesMessage);
            EnqueueMessage(variablesMessage);

            // send diskspace
            EnqueueMessage(TDiskspaceMessage(TargetGraph.WorkerGlobals->VarDirPath.data()));
        }
        break;
    case TCommandMessage::Type:
        {
            TCommandMessage in(*what);

            LOG(MasterAddr << ": Command (" << in.FormatText() << ") received, processing");

            if (!in.GetTarget().empty() && in.GetTask() >= 0) {
                TargetGraph.Command(in.GetFlags(), in.GetTarget(), in.GetTask(), in.GetState());
            } else if (!in.GetTarget().empty()) {
                TargetGraph.Command(in.GetFlags(), in.GetTarget(), in.GetState());
            } else {
                TargetGraph.Command(in.GetFlags(), in.GetState());
            }
        }
        break;
    case TCommandMessage2::Type:
        {
            TCommandMessage2 in(*what);

            LOG(MasterAddr << ": Command2 (" << in.FormatText() << ") received, processing");

            Y_VERIFY(!in.GetTarget().empty(), "Target shouldn't be empty");

            if (in.GetTask().size() > 0) {
                TargetGraph.Command2(in.GetFlags(), in.GetTarget(), in.GetTask());
            } else {
                TargetGraph.Command2(in.GetFlags(), in.GetTarget());
            }
        }
        break;
    case TPokeMessage::Type:
        {
            TPokeMessage in(*what);

            LOG1(net, MasterAddr << ": Poke (" << in.FormatText() << ") received, processing");

            TargetGraph.Poke(in.GetFlags(), in.GetTarget());
        }
        break;
    case TMultiPokeMessage::Type:
        {
            TMultiPokeMessage in(*what);

            LOG1(net, MasterAddr << ": MultiPoke (" << in.FormatText() << ") received, processing");

            TargetGraph.Poke(in.GetFlags(), in.GetTarget(), in.GetTasks());
        }
        break;
    case TVariablesMessage::Type:
        {
            TVariablesMessage in(*what);

            LOG1(net, MasterAddr << ": Variables message (" << in.FormatText() << ") received, processing");

            TargetGraph.ImportVariables(&in);

            LOG1(net, MasterAddr << ": Reflecting variables message");

            // TODO: not logged
            EnqueueMessageImpl(what->Clone());
        }
        break;
    default:
        ythrow yexception() << "unknown message type received: " << (int)what->GetType();
    }
}
