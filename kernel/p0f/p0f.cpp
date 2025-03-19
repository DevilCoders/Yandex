#include "p0f.h"
#include "load/bpf_load.h"
#include "format/p0f_format.h"

#include <library/cpp/resource/resource.h>
#include <util/generic/array_ref.h>
#include <util/generic/cast.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/system/env.h>
#include <util/system/tempfile.h>
#include <util/system/unaligned_mem.h>
#include <bpf.h>

#include <balancer/kernel/helpers/universal_mutex.h>

#include <mutex>

#include <linux/ipv6.h>
#include <netinet/ip.h>
#include <sys/utsname.h>

#include <library/cpp/coroutine/engine/impl.h>

namespace {
    constexpr uint64_t HASH_TIMEOUT = 60 * 1000000000L; // 60 seconds

    constexpr size_t P0F_MAP_ID = 1;
    const TString BpfProgName = "/p0f_bpf";

#define SO_ATTACH_BPF 50 // defined in asm-generic/socket.h

    /// Workaround for iteration over maps. deleteElem used here to reset
    /// state from function after delete finishes.
    static int BpfGetNextAndDelete(int fd, const void* key, void* next_key, bool* deleteElem) {
        int ret = bpf_map_get_next_key(fd, key, next_key);
        if (*deleteElem) {
            bpf_map_delete_elem(fd, key);
            *deleteElem = 0;
        }
        return ret;
    }

    class TBpfProgram {
    public:
        void Load(size_t p0fMaxEntries) noexcept {
            try {
                DoLoad(p0fMaxEntries);
            } catch (...) {
                Cerr << "Failed to load bpf p0f program: " << CurrentExceptionMessage() << Endl;
            }
        }

        void Unload() noexcept {
            try {
                DoUnload();
            } catch (...) {
                Cerr << "Failed to unload bpf p0f program: " << CurrentExceptionMessage() << Endl;
            }
        }

        void Attach(SOCKET s) noexcept {
            if (Loaded) {
                if (setsockopt(s, SOL_SOCKET, SO_ATTACH_BPF, prog_fd, sizeof(prog_fd[0]))) {
                    Cerr << "Failed to attach bpf p0f program to socket: " << LastSystemErrorText() << Endl;
                }
            }
        }

        NP0f::TP0fOrError ExtractFingerprint(const in6_addr& addr, in_port_t srcPort, in_port_t dstPort) noexcept {
            try {
                return DoExtractFingerprint(addr, srcPort, dstPort);
            } catch (...) {
                return NP0f::CreateP0fError("Failed to extract fingerprint: " + CurrentExceptionMessage());
            }
        }

        NP0f::TP0fOrError ExtractFingerprint(const in_addr& addr, in_port_t srcPort, in_port_t dstPort) noexcept {
            try {
                return DoExtractFingerprint(addr, srcPort, dstPort);
            } catch (...) {
                return NP0f::CreateP0fError("Failed to extract fingerprint: " + CurrentExceptionMessage());
            }
        }

        void CleanupMapCoroLock() {
            if (!Loaded) {
                return;
            }

            NSrvKernel::TUniversalGuard guard(Lock_);
            if (!guard.Lock()) {
                return;
            }

            struct timespec uptime;
            int ret = clock_gettime(CLOCK_MONOTONIC, &uptime);
            if (ret != 0) {
                return;
            }
            ui64 ts = uptime.tv_sec * 1000000000 + uptime.tv_nsec;

            char key[20] = {0};
            char prev_key[20] = {0};
            p0f_value_t value = {};
            bool deleteElem = false;
            while (BpfGetNextAndDelete(map_fd[P0F_MAP_ID], &prev_key, &key, &deleteElem) == 0) {
                if (bpf_map_lookup_elem(map_fd[P0F_MAP_ID], &key, &value) == 0) {
                    if (ts - value.ts > HASH_TIMEOUT) {
                        deleteElem = true;
                    }
                }
                memcpy(prev_key, key, sizeof(key));
            }
        }

    private:
        void DoLoad(size_t p0fMaxEntries) {
            if (Loaded) {
                return;
            }

            const auto bpfProgramResource = BpfProgName;
            TString bpfProgramContent = NResource::Find(bpfProgramResource);
            TTempFileHandle bpfFile;
            bpfFile.Write(bpfProgramContent.data(), bpfProgramContent.size());
            bpfFile.Flush();

            P0fMaxEntries = p0fMaxEntries;
            if (load_bpf_file_fixup_map(const_cast<char*>(bpfFile.GetName().data()), FixupMap, nullptr) < 0) {
                ythrow yexception() << "Failed to load p0f bpf program: " << bpf_log_buf;
            }

            Loaded = true;
        }

        void DoUnload() {
            if (!Loaded) {
                return;
            }

            for (int i = 0; i < prog_cnt; ++i) {
                if (close(prog_fd[i]) != 0) {
                    ythrow yexception() << "Failed to unload p0f bpf program";
                }
            }

            Loaded = false;
        }

        static void FixupMap(bpf_map_data* map, int idx, void* context) {
            Y_UNUSED(context);
            if (idx == P0F_MAP_ID) {
                map->def.max_entries = P0fMaxEntries;
            }
        }

        static void FillPorts(char* key, ui16 srcPort, ui16 dstPort) {
            *reinterpret_cast<ui16*>(key + 16) = srcPort;
            *reinterpret_cast<ui16*>(key + 18) = dstPort;
        }

        NP0f::TP0fOrError DoExtractFingerprint(const in6_addr& addr, in_port_t srcPort, in_port_t dstPort) {
            if (!Loaded) {
                return {};
            }

            char key[20];
            p0f_value_t value = {};
            static_assert(sizeof(addr) == 16);
            memcpy(key, reinterpret_cast<const char*>(&addr), sizeof(addr));
            FillPorts(key, srcPort, dstPort);

            if (bpf_map_lookup_elem(map_fd[P0F_MAP_ID], &key, &value) == 0) {
                auto result = NP0f::FormatP0f(value);
                bpf_map_delete_elem(map_fd[P0F_MAP_ID], &key);
                return result;
            }
            return NP0f::CreateP0fError("failed to find p0f");
        }

        NP0f::TP0fOrError DoExtractFingerprint(const in_addr& addr, in_port_t srcPort, in_port_t dstPort) {
            if (!Loaded) {
                return {};
            }

            char key[20] = {};
            p0f_value_t value = {};
            static_assert(sizeof(addr) <= 16);
            memcpy(key + 16 - sizeof(addr), reinterpret_cast<const char*>(&addr), sizeof(addr));
            FillPorts(key, srcPort, dstPort);

            if (bpf_map_lookup_elem(map_fd[P0F_MAP_ID], &key, &value) == 0) {
                auto result = NP0f::FormatP0f(value);
                bpf_map_delete_elem(map_fd[P0F_MAP_ID], &key);
                return result;
            }
            return NP0f::CreateP0fError("failed to find p0f");
        }

    private:
        bool Loaded = false;
        NSrvKernel::TUniversalMutex Lock_;

        static size_t P0fMaxEntries;
    };

    size_t TBpfProgram::P0fMaxEntries = 0;
}

void NP0f::LoadBpfProgram(size_t p0fMaxEntries) noexcept {
    static std::once_flag once{};
    try {
        std::call_once(once, [&]() { Singleton<TBpfProgram>()->Load(p0fMaxEntries); });
    } catch (...) {
        Cerr << "Failed to load bpf p0f program: " << CurrentExceptionMessage() << Endl;
    }
}

void NP0f::UnloadBpfProgram() noexcept {
    Singleton<TBpfProgram>()->Unload();
}

void NP0f::Attach(SOCKET s) noexcept {
    Singleton<TBpfProgram>()->Attach(s);
}

NP0f::TP0fOrError NP0f::ExtractFingerprint(const in6_addr& addr, in_port_t srcPort, in_port_t dstPort) noexcept {
    return Singleton<TBpfProgram>()->ExtractFingerprint(addr, srcPort, dstPort);
}

NP0f::TP0fOrError NP0f::ExtractFingerprint(const in_addr& addr, in_port_t srcPort, in_port_t dstPort) noexcept {
    return Singleton<TBpfProgram>()->ExtractFingerprint(addr, srcPort, dstPort);
}

void NP0f::CleanupMapCoroLock() noexcept {
    Singleton<TBpfProgram>()->CleanupMapCoroLock();
}
