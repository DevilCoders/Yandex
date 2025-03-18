#include "httpsrv.h"

#include <search/cache/filecacher.h>
#include <library/cpp/blockcodecs/codecs.h>

#include <util/thread/factory.h>
#include <util/string/cast.h>
#include <util/digest/city.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <library/cpp/http/misc/parsed_request.h>
#include <util/string/split.h>
#include <library/cpp/digest/md5/md5.h>
#include <util/system/thread.h>
#include <util/system/mutex.h>
#include <util/network/ip.h>
#include <util/ysaveload.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace {
    using NBlockCodecs::ICodec;

    class ICache {
    public:
        virtual ~ICache() {
        }

        virtual bool Load(const TString& key, TString* val) = 0;
        virtual void Store(const TString& key, const TString& val) = 0;
    };

    class TSnappyCompress: public ICache {
    public:
        inline TSnappyCompress(ICache* s)
            : S_(s)
        {
        }

        bool Load(const TString& key, TString* val) override {
            TString res;

            if (S_->Load(key, &res)) {
                *val = Decompress(res);

                return true;
            }

            return false;
        }

        void Store(const TString& key, const TString& val) override {
            S_->Store(key, Compress(val));
        }

    private:
        static inline const ICodec* Codec() {
            static const ICodec* codec = NBlockCodecs::Codec("lz4fast");

            return codec;
        }

        static inline TString Compress(const TString& data) {
            return Codec()->Encode(data);
        }

        inline TString Decompress(const TString& data) {
            return Codec()->Decode(data);
        }

    private:
        ICache* S_;
    };

    class TMemCache: public ICache {
        struct TBucket: public THashMap<TString, TString>, public TMutex {
        };

    public:
        inline TMemCache() {
            for (size_t i = 0; i < 16; ++i) {
                B_.push_back(new TBucket());
            }
        }

        bool Load(const TString& key, TString* val) override {
            TBucket* b = Bucket(key);
            TGuard<TMutex> lock(*b);

            TBucket::const_iterator it = b->find(key);

            if (it != b->end()) {
                *val = it->second;

                return true;
            }

            return false;
        }

        void Store(const TString& key, const TString& val) override {
            TBucket* b = Bucket(key);
            TGuard<TMutex> lock(*b);

            b->insert(std::make_pair(key, val));
        }

    private:
        inline TBucket* Bucket(const TString& key) const noexcept {
            return B_[CityHash64(key) % B_.size()].Get();
        }

    private:
        typedef TVector<TAutoPtr<TBucket> > TBuckets;
        TBuckets B_;
    };

    struct TCacheItem {
        TString Body;
        TString Exception;

        inline TCacheItem() {
        }

        inline TCacheItem(const TString& body, const TString& exc)
            : Body(body)
            , Exception(exc)
        {
        }

        inline void Save(IOutputStream* out) const {
            ::Save(out, Body);
            ::Save(out, Exception);
        }

        inline void Load(IInputStream* in) {
            ::Load(in, Body);
            ::Load(in, Exception);
        }

        inline TString Data() const {
            if (!Exception) {
                return Body;
            }

            ythrow yexception() << Exception;
        }

        inline TString Serialize() const {
            TString ret;
            TStringOutput out(ret);

            ::Save(&out, *this);

            return ret;
        }

        static inline TCacheItem Deserialize(const TString& s) {
            TStringInput in(s);
            TCacheItem ret;

            ::Load(&in, ret);

            return ret;
        }
    };

    class TCache: public ICache {
    public:
        inline TCache(const TString& dir)
            : C_(new TFileCacher(dir))
        {
        }

        bool Load(const TString& key, TString* val) override {
            TAutoPtr<IInputStream> in(C_->Load(key));

            if (!in) {
                return false;
            }

            *val = in->ReadAll();

            return true;
        }

        void Store(const TString& key, const TString& val) override {
            TAutoPtr<IOutputStream> out(C_->Store(key));

            try {
                out->Write(val.data(), val.size());
                C_->Commit(out);
            } catch (...) {
                C_->Discard(out);
                Cerr << CurrentExceptionMessage() << Endl;
            }
        }

    private:
        THolder<TReqCacher> C_;
    };

    class TProxy {
    public:
        inline TProxy(const TString& rhost, ui16 rport, ICache* cache)
            : RemoteHost_(rhost)
            , RemotePort_(rport)
            , RHostID_(RemoteHost_ + ":" + ToString(RemotePort_))
            , Cache_(cache)
            , Addr_(RemoteHost_, RemotePort_)
        {
        }

        static inline TString Md5(TStringBuf str) {
            char buf[33];

            MD5::Data((const unsigned char*)str.data(), str.size(), buf);

            return TString(buf, 32);
        }

        inline TString Key(const TStringBuf& str) const {
            TParsedHttpLocation loc(str);
            TCgiParameters p(loc.Cgi);

            p.EraseAll(TStringBuf("reqid"));
            p.EraseAll(TStringBuf("yandexuid"));
            p.EraseAll(TStringBuf("uid"));
            p.EraseAll(TStringBuf("ruid"));
            p.EraseAll(TStringBuf("push_id"));

            const TString key = RHostID_ + loc.Path + ("?" + p.Print());

            //Cerr << key << Endl;

            return Md5(key);
        }

        inline void Reply(THttpInput& in, THttpOutput& out) {
            TParsedHttpRequest req(in.FirstLine());
            const TString key = Key(req.Request);
            TString ret;

            if (!Cache_->Load(key, &ret)) {
                Cerr << "cache miss: " << RHostID_ + req.Request << Endl;
                TString exception;

                try {
                    ret = Calc(in);
                } catch (...) {
                    exception = CurrentExceptionMessage();
                }

                Cache_->Store(key, TCacheItem(ret, exception).Serialize());
            } else {
                ret = TCacheItem::Deserialize(ret).Data();
            }

            out.Write(ret.data(), ret.size());
        }

        inline TString Calc(THttpInput& in) {
            TSocket s;
            const size_t maxAttempts = 3;

            for (size_t i = 1; i <= maxAttempts; ++i) {
                try {
                    s = TSocket(Addr_, TDuration::Seconds(i));
                } catch (...) {
                    if (i == maxAttempts) {
                        throw;
                    }
                }
            }

            TString req;

            {
                TStringOutput so(req);

                so << in.FirstLine() << "\r\n";
                so << "Host: " << RemoteHost_ << ":" << RemotePort_ << "\r\n";
                so << "Connection: Close" << "\r\n";
                so << "\r\n";
            }

            TSocketOutput(s).Write(req.data(), req.size());

            TString ret;

            {
                TStringOutput so(ret);
                TSocketInput ss(s);

                TransferData(&ss, &so);
            }

            return ret;
        }

    private:
        const TString RemoteHost_;
        const ui16 RemotePort_;
        const TString RHostID_;
        ICache* Cache_;
        TNetworkAddress Addr_;
    };

    typedef TAutoPtr<TProxy> TProxyRef;

    class TServer {
    public:
        inline void Add(ui16 port, TProxyRef proxy) {
            P_[port] = proxy;
        }

        inline void Reply(const NHttp::TRequest& req) {
            Find(InetToHost(req.Local->sin_port))->Reply(*req.In, *req.Out);
        }

    private:
        inline TProxy* Find(ui16 port) const {
            TProxies::const_iterator it = P_.find(port);

            if (it == P_.end()) {
                ythrow yexception() << "no port " << port << " configured";
            }

            return it->second.Get();
        }

    private:
        typedef THashMap<ui16, TProxyRef> TProxies;
        TProxies P_;
    };

    typedef IThreadFactory::IThread IThread;

    class TThreadPoolWithStackSize: public IThreadFactory {
    public:
        class TPoolThread: public IThread {
        public:
            ~TPoolThread() override {
                //Cerr  << "~" << Endl;

                if (!!Thr_) {
                    Thr_->Detach();
                }
            }

            void DoRun(IThreadAble* func) override {
                //Cerr << "run" << Endl;

                Thr_.Reset(new TThread(TThread::TParams(ThreadProc, func, 32 * 1024)));
                Thr_->Start();
            }

            void DoJoin() noexcept override {
                if (!Thr_) {
                    return;
                }

                Thr_->Join();
                Thr_.Destroy();
            }

        private:
            static void* ThreadProc(void* func) {
                ((IThreadAble*)(func))->Execute();

                return nullptr;
            }

        private:
            THolder<TThread> Thr_;
        };

        IThread* DoCreate() override {
            return new TPoolThread();
        }
    };
}

int main(int argc, char** argv) {
    if (argc < 2) {
        Cerr << "usage:\n\tcproxy [-inmem] [localport:remotehost:remoteport]+\n";

        return 0;
    }

    TThreadPoolWithStackSize rpool;

    SetSystemThreadFactory(&rpool);

    try {
        TVector<TAutoPtr<ICache> > caches;

        ++argv;

        if (TStringBuf(*argv) == "-inmem"sv) {
            caches.push_back(new TMemCache());
            ++argv;
        } else {
            caches.push_back(new TCache("./cache"));
        }

        if (1) {
            caches.push_back(new TSnappyCompress(caches.back().Get()));
        }

        ICache* cache = caches.back().Get();

        TServer server;
        THttpServerOptions options;

        options.SetThreads(0);

        while (*argv) {
            TString arg(*argv);
            TVector<TString> parts;

            StringSplitter(arg).Split(':').AddTo(&parts);

            if (parts.size() != 3) {
                ythrow yexception() << "can not parse " << arg.Quote();
            }

            const ui16 port = FromString<ui16>(parts[0]);

            options.AddBindAddress(TString(), port);
            server.Add(port, new TProxy(parts[1], FromString<ui16>(parts[2]), cache));

            ++argv;
        }

        ServeUnqueuedHttp(&server, options);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
    }
}
