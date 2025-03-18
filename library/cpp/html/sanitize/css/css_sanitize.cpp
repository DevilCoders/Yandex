//#include <iostream>
//#include <fstream>
#include <util/stream/output.h>
#include <util/stream/file.h>
#include <util/system/yassert.h>

#include "css_sanitize.h"
#include "sanit_policy.h"
#include "config-drv.h"

namespace Yandex {
    namespace NCssSanitize {
        TCssSanitizer::TCssSanitizer() {
        }

        TCssSanitizer::~TCssSanitizer() {
        }

        void TCssSanitizer::SetTagName(const char* tagName) {
            driver.SetTagName(tagName);
        }

        void TCssSanitizer::SetUrlProcessor(IUrlProcessor* urlProcessor) {
            driver.SetUrlProcessor(urlProcessor);
        }

        void TCssSanitizer::SetProcessAllUrls(bool processAllUrls) {
            driver.SetProcessAllUrls(processAllUrls);
        }

        void TCssSanitizer::SetFilterEntities(IFilterEntities* filterEntities) {
            driver.SetFilterEntities(filterEntities);
        }

        IFilterEntities* TCssSanitizer::GetFilterEntities() {
            return driver.GetFilterEntities();
        }

        int TCssSanitizer::GetErrorCount() const {
            return driver.GetErrorCount();
        }

        void TCssSanitizer::OpenConfig(const TString& the_config_file) {
            NCssConfig::TConfigDriver drv;

            if (!drv.ParseFile(the_config_file))
                throw TCssSanitizerException() << drv.GetParseError();

            config = drv.GetConfig();
        }

        void TCssSanitizer::OpenConfigString(const TString& config_text) {
            NCssConfig::TConfigDriver drv;

            if (!drv.ParseString(config_text, ""))
                throw TCssSanitizerException() << drv.GetParseError();

            config = drv.GetConfig();
        }

        TString TCssSanitizer::Sanitize(const TString& text) {
            if (text.empty())
                return TString();

            NCssSanit::TSanitPolicy policy(config);
            driver.SetPolicy(&policy);

            TStringStream out_stream;
            driver.SetOutStream(out_stream);

            try {
                driver.ParseString(text, "");

                return out_stream.Str();
            } catch (...) {
                Cerr << CurrentExceptionMessage() << Endl;
                return TString();
            }
        }

        TString TCssSanitizer::SanitizeFile(const TString& css_file) {
            NCssSanit::TSanitPolicy policy(config);
            driver.SetPolicy(&policy);
            TStringStream out_stream;
            driver.SetOutStream(out_stream);

            try {
                driver.ParseFile(css_file);
                return out_stream.Str();
            } catch (...) {
                Cerr << CurrentExceptionMessage() << Endl;
                return TString();
            }
        }

        TString TCssSanitizer::SanitizeInline(const TString& style_text) {
            NCssSanit::TSanitPolicy policy(config);
            driver.SetPolicy(&policy);
            TStringStream out_stream;
            driver.SetOutStream(out_stream);

            if (GetFilterEntities() && !GetFilterEntities()->IsAcceptedStyle(style_text.data()))
                return TString();

            try {
                driver.ParseInlineStyle(style_text);
                return out_stream.Str();
            } catch (...) {
                Cerr << CurrentExceptionMessage() << Endl;
                return TString();
            }
        }

        void TCssSanitizer::GetVersion(int* major, int* minor, int* path) {
            if (major)
                *major = VER_MAJOR;

            if (minor)
                *minor = VER_MINOR;

            if (path)
                *path = VER_PATH;
        }

#ifdef WITH_DEBUG_OUT
        void TCssSanitizer::SetDebugOut(bool val) {
            driver.SetDebugOut(val);
        }

        void TCssSanitizer::SetTraceScanning(bool value) {
            driver.SetTraceScanning(value);
        }

        void TCssSanitizer::SetTraceParsing(bool value) {
            driver.SetTraceParsing(value);
        }
#endif
    }
}
