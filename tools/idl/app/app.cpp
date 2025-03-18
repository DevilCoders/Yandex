#include <yandex/maps/idl/app.h>

#include "utils.h"

#include <yandex/maps/idl/config.h>
#include <yandex/maps/idl/env.h>
#include <yandex/maps/idl/generator/cpp/generation.h>
#include <yandex/maps/idl/generator/java/generation.h>
#include <yandex/maps/idl/generator/jni_cpp/generation.h>
#include <yandex/maps/idl/generator/obj_cpp/generation.h>
#include <yandex/maps/idl/generator/objc/generation.h>
#include <yandex/maps/idl/generator/output_file.h>
#include <yandex/maps/idl/generator/protoconv/generator.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/utils/common.h>
#include <yandex/maps/idl/utils/errors.h>
#include <yandex/maps/idl/utils/exception.h>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

#include <exception>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace app {

namespace {

class DefaultDependencyCollector : public DependencyCollector {
public:
    DefaultDependencyCollector(std::ostream& out) : out_(out) {}
    virtual ~DefaultDependencyCollector() { }

    void addIdl(const std::string& idlFileName) override
    {
        out_ << utils::asConsoleBold(idlFileName) << std::endl;
    }

    void addPlatform(const std::string& platformName) override
    {
        out_ << "    " << platformName << ":" << std::endl;
    }

    void addOutput(const std::string& outputFileName) override
    {
        out_ << "        " << outputFileName << std::endl;
    }

    void addInduced(const std::string& /* inducedFileName */) override
    {
        // Do nothing for default collector
    }
private:
    std::ostream& out_;
};

template <typename Generator>
void generateFiles(
    std::vector<generator::OutputFile>& outputFiles,
    const Idl* idl,
    Generator generator,
    bool reportImports,
    DependencyCollector& collector)
{
    for (auto file : generator(idl)) {
        collector.addOutput(file.suffixPath.asString(""));
        if (reportImports) {
            for (const auto& import : file.imports) {
                collector.addInduced(import);
            }
        }
        outputFiles.push_back(std::move(file));
    }
}

void generateFiles(
    std::vector<generator::OutputFile>& outputFiles,
    const Idl* idl,
    bool reportImports,
    DependencyCollector& collector)
{
    const auto& config = idl->env->config;

    // Generate files
    if (!config.outBaseImplRoot.isEmpty()) {
        collector.addPlatform("Base");
        generateFiles(outputFiles, idl, generator::cpp::generate, reportImports, collector);
        generateFiles(outputFiles, idl, generator::protoconv::generate, reportImports, collector);
    }
    if (!config.outAndroidImplRoot.isEmpty()) {
        collector.addPlatform("Android");
        generateFiles(outputFiles, idl, generator::java::generate, reportImports, collector);
        generateFiles(outputFiles, idl, generator::jni_cpp::generate, reportImports, collector);
    }
    if (!config.outIosImplRoot.isEmpty()) {
        collector.addPlatform("iOS");
        generateFiles(outputFiles, idl, generator::objc::generate, reportImports, collector);
        generateFiles(outputFiles, idl, generator::obj_cpp::generate, reportImports, collector);
    }
    if (!reportImports) {
        std::cout << std::endl;
    }
}

void generateFiles(
    Environment* env,
    const std::vector<std::string>& idlPaths,
    bool dryRun,
    bool reportImports,
    DependencyCollector& collector)
{
    if (!reportImports) {
        std::cout << "Parsing..." << std::endl;
    }
    std::vector<const Idl*> idlFiles;
    for (const auto& idlPath : idlPaths) {
        idlFiles.push_back(env->idl(idlPath));
    }

    if (!reportImports) {
        std::cout << "Generating..." << std::endl;
    }
    std::vector<generator::OutputFile> outputFiles;
    for (const Idl* idl : idlFiles) {
        collector.addIdl(idl->relativePath.asString());
        generateFiles(outputFiles, idl, reportImports, collector);
    }

    if (!dryRun) {
        if (!reportImports) {
            std::cout << "Writing..." << std::endl;
        }
        for (const auto& file : outputFiles) {
            file.fullPath().write(file.text);
        }
    }
    if (!reportImports) {
        std::cout << "Done" << std::endl;
    }
}

} // namespace

int run(int argc, const char** argv, DependencyCollector* collector)
{
    utils::installStackTracePrintingSignalHandler();

    DefaultDependencyCollector defaultCollector(std::cout);
    collector = collector ? collector : &defaultCollector;

    const std::string appName = extractAppName(argv);

    namespace po = boost::program_options;

    // Visible options - shown with --help option
    po::options_description visible;
    visible.add(buildHelpDesc());
    visible.add(buildGenerationDesc());

    // All options, including --input-file, which is "positional" and not
    // "visible" from boost::po perspective. It is invisible because users
    // don't need to know about it, they should simply add input files at the
    // end of command line.
    po::options_description all;
    all.add(visible);
    all.add_options()
        (INPUT_FILE, po::value<std::vector<std::string>>(), "input file(s)");
    po::positional_options_description positional;
    positional.add(INPUT_FILE, -1);

    try {
        // Parse arguments
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(all).
            positional(positional).run(), vm);
        po::notify(vm);

        // Run
        if (vm.count(HELP) || argc == 1) {
            std::cout << buildUsageHeading(appName) << visible << std::endl;
        } else if (vm.count(INPUT_FILE)) {
            Environment env(extractConfig(vm));
            generateFiles(&env, extractInputFilePaths(vm), vm.count(DRY_RUN), vm.count(REPORT_IMPORTS), *collector);
        } else {
            throw utils::UsageError() << "No input .idl files specified";
        }
    } catch (const utils::UsageError& e) {
        std::cerr << std::endl << e.what() << std::endl;
        return 1;
    } catch (const boost::program_options::error& e) {
        std::cerr << std::endl << utils::asConsoleBold(e.what()) << std::endl;
        return 2;
    }
    // Any other exception is considered app's internal error, and should
    // not be caught to make debugging easier.

    return 0;
}

} // namespace app
} // namespace idl
} // namespace maps
} // namespace yandex
