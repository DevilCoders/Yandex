## Function tracer
The tracer prints messages to stderr about entering/leaving each instrumented function.

### When to use it
Use this when you have no idea which functions from inside a huge library are used during buggy processing of some data.
The logs may give you a clue where to start.

### How to use it
1. Add `PEERDIR(library/cpp/function_tracer)` to your `CMakeLists.txt`
2. Add `CFLAGS (-finstrument-functions -DENABLE_FUNCTION_TRACER)` to the dir(s) with code to be instrumented.
   Do not try instrumenting `library/cpp/function_tracer`, you'll get stack overflow.
   Put `tracer.cpp` elsewhere if you really feel such a desire.
   This file must not be instrumented because it instantiates C++ templates with no `attribute noinstrument` tag
   (STL does not have such tags by design), so it's not possible to compile this file with `-finstrument-functions` and avoid stack overflow.
   NB currently clang does not support `-finstrument-functions-exclude-file-list`: https://llvm.org/bugs/show_bug.cgi?id=15255
3. Add the following line to your code:
```(C++)
#include <library/cpp/function_tracer/tracer.h>
...
    TFunctionTracer::Instance().Enable = true;
```
to begin instrumentation.
This approach allows to skip instrumentation of static initializers that is currently not supported
(because STL also needs to be statically initialized and is not ready to generate dumps)

4. Optionaly add function names from the dumps that take too much time and lines to
    `TFunctionTracer::Instance().IgnoredFunctionNames` to suppress output for these functions.
    Get the function names directly from the logs unmodified, like that (everything in `«»`):
```
WideToChar(unsigned short const*, unsigned long, ECharset)
unsigned long THashTable<TUtf16String, TUtf16String, THash<TUtf16String>, TIdentity, TEqualTo<TUtf16String>, std::__y1::allocator<TUtf16String> >::bkt_num<TUtf16String>(TUtf16String const&) const
unknown function
```

Also you may add function addresses (seen in the logs as well) to `IgnoredFunctionAddresses`,
but be careful: addresses can change after you add a new item to this map.
So to filter functions by address, first add `nullptr` to the map (or in advance add many `nullptr`s),
then run the program and then replace these nullptrs with addresses of the functions you want to ignore.

You may also add classes from which to ignore all the functions to:
- `IgnoredClasses`: all the functions from the class with exactly this name will be ignored.
   To ignore all global functions, add `""` to IgnoredClasses.
- `IgnoredTemplateClasses`: all the functions from (this class + any class template parameters) will be ignored.
   Namespaces are treated as classes. Note that ignoring `std` does not imply ignoring `std::hash`.
```(C++)
    TFunctionTracer::Instance.IgnoredClasses = { "THashSet<int>" };
    TFunctionTracer::Instance.IgnoredTemplateClasses = { "std::hash", "std" };
```
- `IgnoredPrefixes`: all the functions directly starting with a given prefix are ignored.
   Note that return types generally break this kind of filters.
5. Just run your program as normal. Enjoy

### An example of filters:
```(C++)
TFunctionTracer::Instance().IgnoredClasses = {
    "std",
};
TFunctionTracer::Instance().IgnoredPrefixes = {
    "std::",
};
TFunctionTracer::Instance().IgnoredFunctionNames = {
    "unknown function",
};
TFunctionTracer::Instance().IgnoredTemplateClasses = {
    "__yhashtable_iterator",
    "_allocator_base",
    "_yhashtable_base",
    "_yhashtable_buckets",
    "std::__y1",
    "std::__y1::allocator",
    "std::__y1::allocator_traits",
    "std::__y1::vector",
    "TCowPtr",
    "TDefaultIntrusivePtrOps",
    "THolder",
    "TIntrusiveList",
    "TIntrusiveListItem",
    "TIntrusivePtr",
    "TPointerBase",
    "TPointerCommon",
    "TRefCounted",
    "TSharedPtr",
    "THashMap",
    "THashSet",
    "THashTable",
    "TMap",
    "TVector",
};
```
