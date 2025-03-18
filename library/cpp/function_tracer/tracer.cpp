/*!@file tracer.cpp A file for linking when using -finstrument-functions
 * This file implements the required enter/leave hooks launched for every function.
 * The hooks print messages to stderr about entering/leaving each function.
 * Detailed usage can be found in Readme.md
 */

#include "tracer.h"
#include <util/stream/str.h>

TFunctionTracer& TFunctionTracer::Instance() {
    static TFunctionTracer the(&Cerr);
    return the;
}

#if (defined(__GNUC__) || defined(__clang__)) && !defined(_WIN32)
#if (!defined(NDEBUG) || defined(ENABLE_FUNCTION_TRACER))
#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/stream/format.h>
#include <util/system/type_name.h>
#include <util/system/thread.h>

#include <cstring>

#include <dlfcn.h>
namespace {
    enum class ETracerFunction : bool {
        Enter,
        Leave
    };

    //! Convert function address into a human-readable name
    /*!
     * @note The function uses dlfcn and cxa_demangle to get the most readable name.
     *       Sometimes c++filt and addr2line can still decode more names, but usually this function works.
     *       Decoding more names requires going deeper into DWARF data.
     */
    TString DecodeFunc(void* pfnFuncAddr) {
        Dl_info info;
        memset(&info, 0, sizeof(info));
        auto ok = dladdr(pfnFuncAddr, &info);
        if (!ok || info.dli_sname == nullptr)
            return "unknown function";
        return CppDemangle(info.dli_sname);
    }

    //! Get the TString "[TRACE] (thread xxx) ++++"
    /*! This function counts per-thread nesting level.
     */
    TString GetTracePrefix(ETracerFunction direction) {
        static THashMap<size_t, int> nestingMap;
        static THashMap<size_t, TString> strIds;
        const auto threadId = TThread::CurrentThreadId();
        const TString& strId = [&] {
            if (strIds.count(threadId) == 0) {
                TStringStream ss;
                ss << Hex(threadId);
                strIds[threadId] = ss.Str();
            }
            return strIds[threadId];
        }();
        if (nestingMap.count(threadId) == 0)
            nestingMap[threadId] = 0;
        char filler = (direction == ETracerFunction::Enter) ? '+' : '-';
        int& nesting = nestingMap[threadId];
        if (direction == ETracerFunction::Enter)
            ++nesting;
        TString ans = TString::Join("[TRACE] (thread ", strId, ") ", TString(nesting, filler), " ");
        if (direction == ETracerFunction::Leave)
            --nesting;
        return ans;
    }

    //! Look through funcname until the 'stop' character, skipping <templates>
    /*!@param funcname the line to look through
     * @param i starting index (inclusive) to look from
     * @param stop the character to stop at. Stop characters inside <> are ignored.
     * @param step the loop increment: +1 to look forward, -1 to look backward
     * @return: index before the 'stop' (i.e. (index of 'stop') - step),
     *          -1 if no 'stop' character found
     *
     * @example SkipTemplates ("std::unique_ptr<int>::operator->()", 32, ':', -1)
     *  will return the position of the last ':' in this string, correctly guessing that
     *  'operator->' does not finalize a template definition.
     */
    int SkipTemplates(const TString& funcname, int i, char stop, i8 step) {
        int templateNesting = 0;
        const int limit = step > 0 ? static_cast<int>(funcname.size()) : -1;
        for (; i != limit; i += step) {
            const char& c = funcname[i];
            if (c == '>' &&
                // correctly process 'operator ->' and 'operator >>': not as a template bracket
                // We're relying on demangler that produces nested templates as '> >' (with a space),
                // though it's no longer required.
                (i == 0 || (funcname[i - 1] != '-' && funcname[i - 1] != '>')))
                --templateNesting;
            else if (c == '<')
                ++templateNesting;
            else if (c == stop && templateNesting == 0)
                return i - step;
        }
        return -1;
    };

    //! Given funcname, guess the class it belongs to
    /*!
     * @return "" if no such class detected in the funcname
     *          otherwise return the full class name including template arguments if provided
     */
    TString GetFunctionClass(const TString& funcname) {
        if (!funcname)
            return TString();
        int scopeOp = SkipTemplates(funcname, 0, '(', +1);
        if (scopeOp == -1)
            return TString();
        scopeOp = SkipTemplates(funcname, scopeOp, ':', -1);
        if (scopeOp < 2 || funcname[scopeOp - 2] != ':') //< unexpected ':' that is not a part of "::", giving up
            return TString();
        scopeOp -= 2; //< skip "::"
        int start = SkipTemplates(funcname, scopeOp, ' ', -1);
        if (start < 0)
            start = 0;
        return funcname.substr(start, scopeOp - start);
    }

    //! Given a class name with template args return the class name before the template args.
    /*! @note this function does not strip the leading whitespace. Demangled class name do not
     * have whitespaces before template args, so there is no need to.
     */
    TString StripClassTemplateArgs(const TString& classname) {
        size_t templateArgs = classname.find('<');
        if (templateArgs == TString::npos)
            return classname;
        else
            return classname.substr(0, templateArgs);
    }

    //! Print to stderr a log string for the function func, if this function is not ignored.
    /*!@param func the address of the function we're entering or leaving
     * @param direction do we enter or leave the function.
     *
     * This function checks if func is in the list of ignored addresses,
     *  or if its name is included in IncludeOnly set,
     *  or if its name and its class name is not present in the ignore sets.
     *
     * If the function is ignored, its nesting level is not accounted, no message is printed,
     * and it is added to the list of ignored function addresses to greatly speed up future ignores.
     */
    void DumpTraceLogString(void* func, ETracerFunction direction) {
        auto& tracer = TFunctionTracer::Instance();

        if (!tracer.Enabled || tracer.IgnoredFunctionAddresses.count(func) > 0)
            return;

        const TString& funcname = DecodeFunc(func);
        if (tracer.IncludeOnly && tracer.IncludeOnly.count(funcname) == 0) {
            tracer.IgnoredFunctionAddresses.insert(func); //< fasten next checks
            return;
        }
        if (tracer.IgnoredFunctionNames.count(funcname) > 0) {
            tracer.IgnoredFunctionAddresses.insert(func); //< fasten next checks
            return;
        }
        for (const TString& prefix : tracer.IgnoredPrefixes) {
            if (funcname.StartsWith(prefix)) {
                tracer.IgnoredFunctionAddresses.insert(func); //< fasten next checks
                return;
            }
        }
        if (tracer.IgnoredClasses || tracer.IgnoredTemplateClasses) {
            TString classname = GetFunctionClass(funcname);
            if (tracer.IgnoredClasses.count(classname) > 0) {
                tracer.IgnoredFunctionAddresses.insert(func); //< fasten next checks
                return;
            }
            classname = StripClassTemplateArgs(classname);
            if (tracer.IgnoredTemplateClasses.count(classname) > 0) {
                tracer.IgnoredFunctionAddresses.insert(func); //< fasten next checks
                return;
            }
        }

        (*tracer.Target) << GetTracePrefix(direction) << "«" << funcname << "» (" << func << ")\n";
    }
}

extern "C" {
void __cyg_profile_func_enter(void* func, void* /*caller*/) __attribute__((no_instrument_function));
void __cyg_profile_func_enter(void* func, void* /*caller*/) {
    DumpTraceLogString(func, ETracerFunction::Enter);
}

void __cyg_profile_func_exit(void* func, void* /*caller*/) __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void* func, void* /*caller*/) {
    DumpTraceLogString(func, ETracerFunction::Leave);
}
}
#else  // (!defined(NDEBUG) || defined(ENABLE_FUNCTION_TRACER))
extern "C" {
void __cyg_profile_func_enter(void* func, void* /*caller*/) __attribute__((no_instrument_function));
void __cyg_profile_func_enter(void* /*func*/, void* /*caller*/) {
}

void __cyg_profile_func_exit(void* func, void* /*caller*/) __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void* /*func*/, void* /*caller*/) {
}
}
#endif // (!defined(NDEBUG) || defined(ENABLE_FUNCTION_TRACER))
#else  // defined(__GNUC__) || defined(__clang__)
#pragma message("Function tracer is implemented only for GCC and clang")
// MSVC++ knows nothing about __cyg_profile_ helpers
#endif // defined(__GNUC__) || defined(__clang__)
