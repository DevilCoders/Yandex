//#include <iostream>
//#include <fstream>
#include <util/stream/output.h>
#include <util/stream/file.h>
#include <util/system/yassert.h>

#include "csssanitize.h"
#include "css2-drv.h"
#include "sanit_policy.h"
#include "config-drv.h"

namespace Yandex {
    namespace NCssSanitizer {
        class TCssSanitizerImpl: public ICssSanitizer {
        private:
            TString config_file;
            NCssConfig::TConfig config;

            TString result_string;
            TStringStream out_stream;

            NCssSanit::TCSS2Driver driver;
            TString last_error_text;
            int last_error;

        public:
            TCssSanitizerImpl()
                : last_error(0)
            {
            }

            /* ICssSanitizer implementation */
            virtual void Free();

            virtual int OpenConfig(const char* config_file);

            virtual int OpenConfigString(const char* config_text);

            virtual const char* Sanitize(const char* css_text);

            virtual const char* GetLastErrorString();

            virtual int GetLastError() {
                return last_error;
            }
        };

        static const char* error_text[] = {
            "Success",
            "Out of memory",
            "Parse error",
            "Bad argument of function"};

        int TCssSanitizerImpl::OpenConfig(const char* config_file) {
            Y_ASSERT(this != 0 && config_file != 0);

            if (!config_file) {
                last_error_text = error_text[E_BADARGUMENT];
                last_error = E_BADARGUMENT;
                return E_BADARGUMENT;
            }

            try {
                NCssConfig::TConfigDriver drv;

                if (!drv.ParseFile(config_file)) {
                    last_error_text = drv.GetParseError();
                    return last_error = E_PARSE_ERROR;
                }

                config = drv.GetConfig();

                last_error = 0;
                last_error_text = "";

                return E_SUCCESS;
            } catch (std::exception& ex) {
                last_error = E_OTHER;
                last_error_text = ex.what();
                return E_OTHER;
            }
        }

        int TCssSanitizerImpl::OpenConfigString(const char* config_text) {
            Y_ASSERT(this != 0);
            Y_ASSERT(config_text != 0);

            if (!config_text) {
                last_error_text = error_text[E_BADARGUMENT];
                last_error = E_BADARGUMENT;
                return E_BADARGUMENT;
            }

            try {
                NCssConfig::TConfigDriver drv;

                if (!drv.ParseString(config_text, "")) {
                    last_error_text = drv.GetParseError();
                    return last_error = E_PARSE_ERROR;
                }

                config = drv.GetConfig();

                last_error = 0;
                last_error_text = "";

                return E_SUCCESS;
            } catch (std::exception& ex) {
                last_error = E_OTHER;
                last_error_text = ex.what();
                return E_OTHER;
            }
        }

        const char* TCssSanitizerImpl::Sanitize(const char* text) {
            Y_ASSERT(this != 0 && text != 0);

            if (!text) {
                last_error_text = error_text[E_BADARGUMENT];
                last_error = E_BADARGUMENT;
                return 0;
            }

            try {
                NCssSanit::TSanitPolicy policy(config);
                driver.SetPolicy(&policy);
                driver.SetOutStream(out_stream);

                driver.ParseString(text, "");

                last_error = E_SUCCESS;
                last_error_text = "";

                result_string = out_stream.Str();
                return result_string.c_str();

            } catch (std::exception& ex) {
                last_error = E_OTHER;
                last_error_text = ex.what();
                result_string = "";
                return 0;
            }
            return result_string.c_str();
        }

        const char* TCssSanitizerImpl::GetLastErrorString() {
            return last_error_text.c_str();
        }

        void TCssSanitizerImpl::Free() {
            Y_ASSERT(this != 0);

            delete this;
        }

    }
}

extern "C" {
void CssSanitizerVersion(int* major, int* minor, int* path) {
    *major = VER_MAJOR;
    *minor = VER_MINOR;
    *path = VER_PATH;
}

Yandex::NCssSanitizer::ICssSanitizer* CssSanitizer_Create() {
    Yandex::NCssSanitizer::ICssSanitizer* res = 0;
    try {
        res = new Yandex::NCssSanitizer::TCssSanitizerImpl;
    } catch (...) {
        res = 0;
    }

    return res;
}

} //extern "C"
