#pragma once

namespace Yandex {
    namespace NCssSanitizer {
        /* Error codes */
        const int E_SUCCESS = 0;
        const int E_OUTMEMORY = 1;
        const int E_PARSE_ERROR = 2;
        const int E_BADARGUMENT = 3;
        const int E_OTHER = 4;

        struct ICssSanitizer {
            /** Free the instance
     * @return      nothing
     */
            virtual void Free() = 0;

            /** Open sanitizer configuration from file
     *  It is not required to call this func - default config will be used
     *
     *  @param config_file      path to config
     *  @return                 zero if success, or error number
     */
            virtual int OpenConfig(const char* config_file) = 0;

            /** Open sanitizer configuration from string
     *  It is not required to call this func -
     *  default config will be used
     *
     *  @param config_file      path to config
     *  @return                 zero if success, or error number
     */
            virtual int OpenConfigString(const char* config_text) = 0;

            /** Do sanitize
     *  @param handle       handle of sanitizer created by CssSanitizerOpen()
     *  @param css_text     text to sanitize
     *  @return             pointer to C-string result. if css_text is NULL, return value is zero
     */
            virtual const char* Sanitize(const char* css_text) = 0;

            /** Return a text of last error
     * @return      pointer to C-string with last error description
     */
            virtual const char* GetLastErrorString() = 0;

            /* Return a code of last error
     * @return      code of last error
     */
            virtual int GetLastError() = 0;
        };
    }
} //namespace Yandex

extern "C" {
/** Return version info
     *  @param major     major number
     *  @param minor     minor number
     *  @param path      path number
     *  @return          nothing
     */
void CssSanitizerVersion(int* major, int* minor, int* path);

/** Create new sanitizer instance
     *  @return: handle of sanitizer instance or NULL if memory error
     */
Yandex::NCssSanitizer::ICssSanitizer* CssSanitizer_Create();

} // extern "C"
