#include <util/thread/lfqueue.h>
#include <library/cpp/http/server/http.h>

struct TServer: public THttpServer, public THttpServer::ICallBack {
    class TReply: public TClientRequest {
        TString Response;
        TServer* Parent;

        bool Reply(void*) override {
            if (Response) {
                Cout << "really send reply" << Endl;
                Output() << Response;

                return true;
            }

            Parent->Queue.Enqueue(this);

            return false;
        }

    public:
        inline TReply(TServer* parent)
            : Parent(parent)
        {
        }

        ~TReply() override {
            Cout << "destory context" << Endl;
        }

        inline void SendResponse(const TString& r) {
            Response = r;
            static_cast<IObjectInQueue*>(this)->Process(nullptr);
        }
    };

    inline TServer()
        : THttpServer(this, TOptions().SetThreads(5).SetHost("::1").SetPort(18000))
    {
    }

    TClientRequest* CreateClient() override {
        Cout << "create client" << Endl;

        return new TReply(this);
    }

    inline TReply* Dequeue() {
        TReply* ret;

        while (!Queue.Dequeue(&ret)) {
            usleep(1000);
        }

        return ret;
    }

    TLockFreeQueue<TReply*> Queue;
};

int main() {
    TServer srv;

    srv.Start();

    while (true) {
        Cout << "wait req" << Endl;
        auto r = srv.Dequeue();
        Cout << "sleep" << Endl;
        // big fat calc
        sleep(2);
        Cout << "send reply" << Endl;
        //r is destroyed after this call
        r->SendResponse("HTTP/1.1 200 Ok\r\n\r\nkekeke");
    }
}
