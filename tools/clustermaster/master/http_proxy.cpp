#include "http.h"
#include "http_common.h"
#include "master.h"
#include "master_target_graph.h"
#include "worker_pool.h"

#include <util/generic/bt_exception.h>

void TMasterHttpRequest::ServeProxy(IOutputStream& out0, const TString& workerHost, const TString& url) {
    TBufferedOutput out(&out0);

    THolder<TLockableHandle<TWorkerPool>::TReadLock> pool(new TLockableHandle<TWorkerPool>::TReadLock(GetPool()));
    TWorkerPool::TWorkersList::const_iterator worker = (*pool)->GetWorkers().find(workerHost);

    if (worker == (*pool)->GetWorkers().end()) {
        ServeSimpleStatus(out0, HTTP_BAD_REQUEST, TString("Wrong worker host ") + EncodeXMLString(workerHost.data()));
        return;
    }

    if (!(*worker)->GetHttpAddress()) {
        ServeSimpleStatus(out0, HTTP_SERVICE_UNAVAILABLE, "Worker is not active");
        return;
    }

    try {
        TSocket sock(*(*worker)->GetHttpAddress());

        pool.Destroy();

        sock.SetSocketTimeout(MasterOptions.ProxyHttpTimeout, 0);

        TSocketOutput sockOut(sock);
        TSocketInput sockIn(sock);

        // Send request
        sockOut << TString("GET ") + url + " HTTP/1.1\r\n";

        // Send headers
        for (auto i = ParsedHeaders.begin(); i != ParsedHeaders.end(); ++i) {
            // name
            sockOut << i->first << ":";

            // value
            sockOut << i->second;

            sockOut << "\r\n";
        }
        sockOut << "\r\n";

        sockOut.Finish();

        out << sockIn.ReadAll();
    } catch (const yexception& e) {
        ServeSimpleStatus(out0, HTTP_INTERNAL_SERVER_ERROR, TString("Error connecting to worker: ") + EncodeXMLString(e.what()));
        return;
    }

    out.Flush();
}

void TMasterHttpRequest::ServeThrow(IOutputStream&) {
    ythrow TWithBackTrace<yexception>() << "sample exception";
}

