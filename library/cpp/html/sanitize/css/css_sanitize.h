#pragma once

#include <util/generic/yexception.h>

#include "css2-drv.h"
#include "sanit_config.h"

/** Top level interface to CSS Sanitizer
  */

namespace Yandex {
    namespace NCssSanitize {
        class TCssSanitizerException: public yexception {
        };

        class TCssSanitizer {
        private:
            TString config_file;
            NCssConfig::TConfig config;

            TString result_string;

            NCssSanit::TCSS2Driver driver;

        public:
            TCssSanitizer();
            ~TCssSanitizer();

            void SetTagName(const char* tagName);
            void SetUrlProcessor(IUrlProcessor* urlProcessor);
            void SetProcessAllUrls(bool processAllUrls);
            void SetFilterEntities(IFilterEntities* filterEntities);
            IFilterEntities* GetFilterEntities();
            /** Open sanitizer configuration from file
     *  It is not required to call this func -
     *  default config will be used
     *
     *  @param config_file      path to config file
     *  @return                 none
     */
            void OpenConfig(const TString& config_file);

            /** Open sanitizer configuration from string
     *  It is not required to call this func -
     *  default config will be used
     *
     *  @param config_text      config text
     *  @return                 none
     */
            void OpenConfigString(const TString& config_text);

            /** Do sanitize
     *  @param handle       handle of sanitizer created by CssSanitizerOpen()
     *  @param css_text     text to sanitize
     *  @return             TString result. if css_text is NULL, returned string is is zero
     */
            TString Sanitize(const TString& css_text);
            TString SanitizeFile(const TString& css_file);
            TString SanitizeInline(const TString& style_text);

            int GetErrorCount() const;

            static void GetVersion(int* major, int* minor, int* path);

#ifdef WITH_DEBUG_OUT
            void SetDebugOut(bool val);
            void SetTraceScanning(bool value);
            void SetTraceParsing(bool value);
#endif
        };

    }
} //namespace Yandex
