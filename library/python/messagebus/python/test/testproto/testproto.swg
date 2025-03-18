%include <typemaps.i>
%include <python/exception.i>
%include <bindings/swiglib/stroka.swg>
%include <bindings/swiglib/defaults.swg>

%include "../../../messagebus.swg"

%begin %{
    #include <util/generic/string.h>
    #include <util/generic/yexception.h>
    #include <utility>
    #include <library/python/messagebus/python/test/testproto/wrapper.h>

    #ifdef __clang__
    #pragma clang diagnostic ignored "-Wself-assign"
    #endif
%}

%include "wrapper.h"
