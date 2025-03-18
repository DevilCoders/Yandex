#include <yandex/maps/idl/generator/java/generation.h>
#include <yandex/maps/idl/generator/jni_cpp/generation.h>
#include <yandex/maps/idl/generator/objc/generation.h>
#include <yandex/maps/idl/generator/cpp/generation.h>
#include <yandex/maps/idl/generator/obj_cpp/generation.h>
#include <yandex/maps/idl/idl.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>

#include <boost/filesystem.hpp>

#include <array>
#include <iostream>

using namespace yandex::maps::idl;
using namespace yandex::maps::idl::generator;

constexpr auto IDL_FRAMEWORKS_ROOT = "idl-frameworks-root";
constexpr auto IDL_FILES_ROOT = "idl-files-root";
constexpr auto GENERATED_FILES_ROOT = "generated-files-root";

class Main: public TMainClassArgs {
    std::string idlFrameworksRoot_;
    std::string idlFilesRoot_;
    std::string generatedFilesRoot_;
protected:
    void RegisterOptions(NLastGetopt::TOpts& opts) override
    {
        opts.SetTitle("generated_files_updater -- speaks for itself");

        opts.AddHelpOption('h');

        opts.AddLongOption('f', IDL_FRAMEWORKS_ROOT)
            .DefaultValue("../idl_frameworks")
            .Help("where to search for .framework files")
            .StoreResult(&idlFrameworksRoot_);

        opts.AddLongOption('i', IDL_FILES_ROOT)
            .DefaultValue("../idl_files")
            .Help("where to search for .idl files")
            .StoreResult(&idlFilesRoot_);

        opts.AddLongOption('g', GENERATED_FILES_ROOT)
            .DefaultValue("../generated_files")
            .Help("where to put generated files")
            .StoreResult(&generatedFilesRoot_);
    }

    int DoRun(NLastGetopt::TOptsParseResult&& /*parsedOptions*/) override
    {
        auto env = Environment({
            "", // inProtoRoot
            { idlFrameworksRoot_ },
            { idlFilesRoot_ },
            "", // baseProtoPackage
            "",
            "",
            "",
            "",
            "",
            "",
            false,
            false,
            false
        });

        auto generators = {generator::cpp::generate,
                           generator::java::generate,
                           generator::jni_cpp::generate,
                           generator::objc::generate,
                           generator::obj_cpp::generate};

        for (boost::filesystem::recursive_directory_iterator end, dir("../idl_files/"); dir != end; ++dir ) {
            if (!dir->path().has_extension())
                continue;
            std::string p = dir->path().string();
            // erase `../idl_files/` path from idl filename
            std::string idl_file = p.erase(0, 13);
            auto idl = env.idl(idl_file);

            for (auto generator: generators) {
                for (auto file: generator(idl)) {
                    try {
                        utils::Path(
                            generatedFilesRoot_
                            + file.suffixPath.fileName()
                        ).write(file.text);
                    } catch (utils::UsageError& error) {
                        std::cerr << "File write error: " << error.what();
                    }
                }
            }
        }
        return 0;
    }
};


int main(int argc, const char** argv)
{
    Main().Run(argc, argv);
    return 0;
}
