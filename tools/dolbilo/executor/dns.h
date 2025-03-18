#pragma once

#include <util/generic/ptr.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/network/socket.h>

class IDns {
    public:
        typedef TAtomicSharedPtr<TNetworkAddress> TAddrRef;

        inline IDns() noexcept {
        }

        virtual ~IDns() {
        }

        virtual TAddrRef Resolve(const TString& host, ui16 port) = 0;
};

class TCachedDns: public IDns {
        typedef std::pair<TString, ui16> TEndpoint;
        typedef THashMap<TEndpoint, TAddrRef> TIpCache;
    public:
        class TSlaveHolder {
            public:
                inline TSlaveHolder(TCachedDns* dns, IDns* next)
                    : Dns_(dns)
                    , Prev_(Dns_->SetSlave(next))
                {
                }

                inline ~TSlaveHolder() {
                    Dns_->SetSlave(Prev_);
                }

            private:
                TCachedDns* Dns_;
                IDns* Prev_;
        };

        inline TCachedDns(IDns* slave) noexcept
            : Slave_(slave)
        {
        }

        ~TCachedDns() override {
        }

        TAddrRef Resolve(const TString& host, ui16 port) override {
            TIpCache::const_iterator it = Cache_.find(TEndpoint(host, port));

            if (it != Cache_.end()) {
                return it->second;
            }

            TAddrRef ip(Slave_->Resolve(host, port));

            if (ip) {
                Cache_[TEndpoint(host, port)] = ip;
            }

            return ip;
        }

        inline IDns* SetSlave(IDns* slave) noexcept {
            IDns* ret = Slave_; Slave_ = slave; return ret;
        }

    private:
        IDns* Slave_;
        TIpCache Cache_;
};

class TSyncDns: public IDns {
    public:
        inline TSyncDns() noexcept {
        }

        ~TSyncDns() override {
        }

        TAddrRef Resolve(const TString& host, ui16 port) override {
            TAddrRef ret;
            try {
                ret.Reset(new TNetworkAddress(host, port));
            } catch (...) {
                ;
            }
            return ret;
        }
};

class TContExecutor;

class TAsyncDns: public IDns {
    public:
        TAsyncDns(TContExecutor* e) noexcept;
        TAsyncDns(TContExecutor* e, size_t maxreqs) noexcept;
        ~TAsyncDns() override;

        TAddrRef Resolve(const TString& host, ui16 port) override;

    private:
        class TImpl;
        THolder<TImpl> Impl_;
};
