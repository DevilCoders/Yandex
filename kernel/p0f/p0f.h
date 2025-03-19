#pragma once

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/network/init.h>

#include <kernel/p0f/format/p0f_format.h>

namespace NP0f {
    void LoadBpfProgram(size_t p0fMaxEntries) noexcept;
    void UnloadBpfProgram() noexcept;

    void Attach(SOCKET s) noexcept;

    TP0fOrError ExtractFingerprint(const in6_addr& addr, in_port_t srcPort, in_port_t dstPort) noexcept;
    TP0fOrError ExtractFingerprint(const in_addr& addr, in_port_t srcPort, in_port_t dstPort) noexcept;
    void CleanupMapCoroLock() noexcept;
}
