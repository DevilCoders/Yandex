#pragma once

#include <string>

class DependencyCollector {
public:
    virtual ~DependencyCollector() { }

    virtual void addIdl(const std::string& idlFileName) = 0;
    virtual void addPlatform(const std::string& platformName) = 0;
    virtual void addOutput(const std::string& outputFileName) = 0;
    virtual void addInduced(const std::string& inducedFileName) = 0;
};

namespace yandex {
namespace maps {
namespace idl {
namespace app {

int run(int argc, const char** argv, DependencyCollector* collector = nullptr);

} // namespace app
} // namespace idl
} // namespace maps
} // namespace yandex
