#pragma once

#include <util/generic/hash_set.h>
#include <util/generic/string.h>

class IOutputStream;

//! Tracer controls
struct TFunctionTracer {
public:
    static TFunctionTracer& Instance();

public:
    //! Set this flag to true to start printing
    bool Enabled = false;
    //! Ignore functions with full names exactly matching one of IgnoredFunctionNames
    THashSet<TString> IgnoredFunctionNames;
    //! If this property is set, then only functions with names exactly found in this set are printed
    THashSet<TString> IncludeOnly;
    //! Addresses of ignored functions
    /*!  Can be filled manually, but it's tricky:
     *    1. add nullptrs to this set
     *    2. run the program
     *    3. replace nullptrs with the addresses you need.
     *
     * This structure is filled automatically with addresses of all previously ignored functions.
     * If you want to stop ignoring a particular function, you should not only remove it from an Ignored* field,
     * but also remove its address from here (probably, using just .clear())
     */
    THashSet<void*> IgnoredFunctionAddresses;
    //! Ignore all functions that have classname:: at the beginning of the full function name for each classname in IgnoredClasses.
    /*! @note All nested classes of an ignored class are not ignored. Namespaces are also considered as classes for this purpose. */
    THashSet<TString> IgnoredClasses;
    //! Ignore all functions that have classname<...> at the beginning of the full function name for each classname in IgnoredTemplateClasses
    /*! @note All nested classes of an ignored class are not ignored. Namespaces are also considered as classes for this purpose. */
    THashSet<TString> IgnoredTemplateClasses;

    //! The functions with names starting exactly with given prefixes are completely ignored
    THashSet<TString> IgnoredPrefixes;

    //! Where to write the logs
    IOutputStream* Target /* = &Cerr */;

private:
    TFunctionTracer(IOutputStream* target)
        : Target(target)
    {
    }
};
