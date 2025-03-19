#pragma once

#include "hostinfo.h"

#include <library/cpp/deprecated/atomic/atomic.h>

#include <util/network/endpoint.h>
#include <util/generic/ptr.h>
#include <util/generic/deque.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>

enum EConnDataStat {
    CDS_ConnSucceeded,
    CDS_ConnFailed,
    CDS_ReqSucceeded,
    CDS_ReqSkipped,
    CDS_ReqFailed,
//add new here
    CDS_Size
};

namespace NHttpSearchClient {
    struct TAddress {
        TString RealAddress; //may be with ip
        TString LoggedAddress; //with hostname
    public:
        TAddress& AddSuffix(TStringBuf s) {
            RealAddress += s;
            LoggedAddress += s;
            return *this;
        }
    };
}

class TConnGroup {
    public:
        class TConnStat {
        public:
            TConnStat() {
                for (size_t i = 0; i < CDS_Size; ++i) {
                    Stats_[i] = 0;
                }
            }

            void Inc(EConnDataStat id) {
                AtomicIncrement(Stats_[id]);
                if (id == CDS_ConnFailed || id == CDS_ReqFailed) {
                    AtomicSet(Acc_, 0);
                } else if (id == CDS_ConnSucceeded) {
                    AtomicSet(Acc_, 1);
                }
            }

            size_t Get(EConnDataStat id) const {
                return (size_t)Stats_[id];
            }

            size_t Accessibility() const {
                return (size_t)AtomicGet(Acc_);
            }

        private:
            TAtomic Stats_[CDS_Size];
            TAtomic Acc_ = 1;
        };

        class TConnData: public TConnStat {
            public:
                class IConnDataExtension {
                public:
                    virtual ~IConnDataExtension();
                };
                TConnData(const TString& script, bool enableIpV6, bool enableUnresolvedHostname
                          , bool enableCachedResolve
                          , TString id, bool isMain, TConnGroup* parent = nullptr, const TString& ip = {});

                void UpdateIndexGeneration(ui32 indGen);
                void UpdateSourceTimestamp(ui32 ts);
                void Register();

                inline bool Registered() const noexcept {
                    return Registered_;
                }
                inline bool IsMain() const noexcept {
                    return Main_;
                }
                inline const TEndpoint& Endpoint() const noexcept {
                    return EndpointData_.Endpoint;
                }
                inline const TString& Ip() const {
                    if (Scheme_.EndsWith("+unix")) {
                        return Host_;
                    }
                    return EndpointData_.Ip;
                }
                inline ui16 Port() const noexcept {
                    return EndpointData_.Port;
                }
                inline bool HasUnresolvedHost() const {
                    return HasUnresolvedHost_;
                }
                inline ui32 IndexGeneration() const {
                    return AtomicGet(IndexGeneration_);
                }
                inline ui32 SourceTimestamp() const {
                    return AtomicGet(SourceTimestamp_);
                }

                const TString& SearchScript() const {
                    return SearchScript_;
                }
                const TString& Scheme() const {
                    return Scheme_;
                }
                const TString& Host() const {
                    return Host_;
                }
                const TString& Path() const {
                    return Path_;
                }
                const TString& GroupDescr() const {
                    return GroupDescr_;
                }
                const NHttpSearchClient::TAddress& Address() const {
                    return Address_;
                }
                IConnDataExtension* GetExtension() const noexcept {
                    return Extension_;
                }
                void SetExtension(IConnDataExtension* ext) noexcept {
                    Extension_ = ext;
                }

                static TEndpoint::TAddrRef GetFirstAddr(const TNetworkAddress& addr, bool enableIpV6);

            private:
                TString SearchScript_;

                /*
                 * parsed data
                 */
                TString Scheme_;
                TString Host_;
                TString Path_;

                TString GroupDescr_;

                TConnGroup* Parent_ = nullptr;
                bool Registered_ = false;
                bool Main_ = false;

                struct TEndpointData {
                public:
                    TEndpointData() = default;
                    TEndpointData(const TEndpoint& endpoint);
                public:
                    TEndpoint Endpoint;
                    ui16 Port = 0;
                    TString Ip;
                } EndpointData_;

                bool HasUnresolvedHost_ = true;

                TAtomic IndexGeneration_ = UndefIndGenValue;
                TAtomic SourceTimestamp_ = 0;

                NHttpSearchClient::TAddress Address_;
                TMaybe<TNetworkAddress> NetworkAddress_;

                IConnDataExtension* Extension_ = nullptr;
        };

        TConnGroup() {
        }

        inline void RegisterConnData(const TConnData* connData) {
            ConnDatas_.push_back(connData);
        }

        inline void UpdateIndexGeneration() {
            TAtomicBase ig = RecalcClientIndexGeneration();
            AtomicSet(ClientIndexGeneration_, ig);
        }

        inline void UpdateSourceTimestamp() {
            TAtomicBase ts = RecalcClientSourceTimestamp();
            AtomicSet(ClientSourceTimestamp_, ts);
        }

        inline ui32 IndexGeneration() const noexcept {
            return AtomicGet(ClientIndexGeneration_);
        }

        inline ui32 SourceTimestamp() const noexcept {
            return AtomicGet(ClientSourceTimestamp_);
        }

        inline double Accessibility() const noexcept {
            size_t accCount = 0;
            for (TConnDataList::const_iterator it = ConnDatas_.begin(); it != ConnDatas_.end(); ++it) {
                accCount += (*it)->Accessibility();
            }

            return ConnDatas_.size() > 0 ? (double)accCount / ConnDatas_.size() : 1;
        }

    private:
        ui32 RecalcClientIndexGeneration();
        ui32 RecalcClientSourceTimestamp();

    private:
        typedef TVector<const TConnData*> TConnDataList;
        TConnDataList ConnDatas_;
        TAtomic ClientIndexGeneration_ = UndefIndGenValue;
        TAtomic ClientSourceTimestamp_ = 0;
};

typedef TConnGroup::TConnData TConnData;


namespace NHttpSearchClient {
    TAddress MakeAddress(const TConnData& baseConnData, TStringBuf scheme, ui32 port, TStringBuf suffix);
}

class IConnIterator {
    public:
        typedef NHttpSearchClient::THostInfo THostInfo;

        enum ERequestStatus {
            RS_Failed,
            RS_TimedOut,
            RS_Ok
        };
        struct TErrorDetails {
            TDuration RequestDuration;
            ERequestStatus Status = RS_Ok;
            i32 SystemErrorCode = 0;
            TString ErrorMessage;
        };

        inline IConnIterator() noexcept {
        }

        virtual ~IConnIterator() {
        }

        virtual const TConnData* Next(size_t attempt) = 0;
        void SetClientStatus(const TConnData* conn, ui64 answerTime, const THostInfo& props) {
            TErrorDetails details;
            if (answerTime > 0) {
                details.RequestDuration = TDuration::MicroSeconds(answerTime);
            } else {
                details.Status = RS_Failed;
            }
            SetClientStatus(conn, props, details);
        }
        virtual void SetClientStatus(const TConnData* conn, const THostInfo& props, const TErrorDetails&) = 0;
};

void UpdateConnData(const TConnData* conn, const IConnIterator::THostInfo& props);

class TFilterIteratorBase : public IConnIterator {
    public:
        TFilterIteratorBase(THolder<IConnIterator> iter)
            : SlaveIt_(std::move(iter))
        {
        }

        ~TFilterIteratorBase() override {
        }

        void SetClientStatus(const TConnData* conn, const THostInfo& props, const TErrorDetails& errorDetails) override {
            SlaveIt_->SetClientStatus(conn, props, errorDetails);
        }

        IConnIterator* Slave() const {
            return SlaveIt_.Get();
        }

    private:
        THolder<IConnIterator> SlaveIt_;
};

template<class TConnFilter>
class TSelectIterator: public TFilterIteratorBase {
    public:
        TSelectIterator(THolder<IConnIterator> iter, TConnFilter filter)
            : TFilterIteratorBase(std::move(iter))
            , Filter_(filter)
        {
        }

        ~TSelectIterator() override {
        }

        const TConnData* Next(size_t attempt) override {
            const TConnData* res = nullptr;

            do {
                res = Slave()->Next(attempt);
            } while(!!res && !Filter_.Allowed(res));

            return res;
        }

    private:
        TConnFilter Filter_;
};

template<class TConnFilter>
class TSkipIterator: public TFilterIteratorBase {
    public:
        TSkipIterator(THolder<IConnIterator> iter, TConnFilter filter)
            : TFilterIteratorBase(std::move(iter))
            , Filter_(filter)
        {
        }

        ~TSkipIterator() override {
        }

        const TConnData* Next(size_t /*attempt*/) override {
            while (const TConnData* res = Slave()->Next(Attempt_)) {
                ++Attempt_;
                if (Filter_.Allowed(res)) {
                    return res;
                } else {
                    SkippedItems_.push_back(res);
                }
            }

            if (!SkippedItems_.empty()) {
                const TConnData* res = SkippedItems_.front();
                SkippedItems_.pop_front();
                return res;
            }

            return nullptr;
        }
    private:
        TDeque<const TConnData*> SkippedItems_;
        TConnFilter Filter_;
        size_t Attempt_ = 0;
};

class TConnDataFilter {
public:
    TConnDataFilter(const TConnData* connData)
        : ConnData_(connData)
    {
    }

    bool Allowed(const TConnData* connData) const {
        return ConnData_ == connData;
    }
private:
    const TConnData* ConnData_ = nullptr;
};

class TConnIterator {
public:
    typedef IConnIterator::THostInfo THostInfo;
    static const size_t NoAttempt = (size_t)(-1);
public:
    explicit TConnIterator(THolder<IConnIterator> iter)
        : Iter_(std::move(iter))
    {
    }

    bool AtEnd() const {
        return (nullptr == CurrentConn_ && Attempt_ != NoAttempt) || !Iter_;
    }

    const TConnData* operator->() const {
        Y_ASSERT(CurrentConn_);
        return CurrentConn_;
    }

    const TConnData& operator*() const {
        Y_ASSERT(CurrentConn_);
        return *CurrentConn_;
    }

    const TConnData* Next() {
        return CurrentConn_ = Iter_->Next(++Attempt_);
    }

    void SetClientStatus(const TConnData* conn, ui64 answerTime, const THostInfo& props) {
        return Iter_->SetClientStatus(conn, answerTime, props);
    }

    void SetClientStatus(const TConnData* conn, const THostInfo& props, const IConnIterator::TErrorDetails& errorDetails) {
        Iter_->SetClientStatus(conn, props, errorDetails);
    }

    size_t Attempt() const {
        return Attempt_;
    }

    THolder<IConnIterator> Release() {
        return std::move(Iter_);
    }
private:
    THolder<IConnIterator> Iter_;
    const TConnData* CurrentConn_ = nullptr;
    size_t Attempt_ = NoAttempt;
};
