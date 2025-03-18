#include <library/cpp/coroutine/dns/coro.h>
#include <library/cpp/coroutine/dns/helpers.h>

#include <library/cpp/coroutine/engine/impl.h>

#include <util/network/address.h>
#include <util/network/hostip.h>

using namespace NAddr;
using namespace NAsyncDns;

static TContResolver* resolver = nullptr;

static inline void Func(TCont*, void* arg) {
    const TString addr((const char*)arg);

    try {
        for (size_t i1 = 0; i1 < 1; ++i1) {
            TAddrs res;

            ResolveAddr(*resolver, addr, res);

            if (i1 == 0) {
                for (size_t i2 = 0; i2 < res.size(); ++i2) {
                    Cout << addr << " -> " << *res[i2] << Endl;
                }
            }
        }
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
    }
}

int main() {
    const char* names[] = {
        "yandex.ru", "yandex.com", "yandex.com.tr", "google.com", "google.ru", "market.yandex.ru"};

    if (1) {
        TContExecutor e(8192 * 2);
        TContResolver res(&e, TOptions().SetMaxRequests(100));

        resolver = &res;

        for (auto& name : names) {
            e.Create(Func, (void*)name, name);
        }

        e.Execute();
    } else {
        for (auto& name : names) {
            TNetworkAddress addr(name, 80);

            for (TNetworkAddress::TIterator it = addr.Begin(); it != addr.End(); ++it) {
                Cerr << name << " -> " << (const IRemoteAddr&)TAddrInfo(&*it) << Endl;
            }
        }
    }
}
