%module filestorage
%include <typemaps.i>
%include <python/exception.i>
%include <bindings/swiglib/stroka.swg>
%include <bindings/swiglib/defaults.swg>

%begin %{
    #include <tools/snipmake/steam/filestorage/lib/storage.h>
%}
%include <tools/snipmake/steam/filestorage/lib/storage.h>

