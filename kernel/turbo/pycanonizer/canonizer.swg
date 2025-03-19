%include <typemaps.i>
%include <bindings/swiglib/defaults.swg>
%include <bindings/swiglib/stroka.swg>
%{
    #include <kernel/turbo/canonizer/canonizer.h>
%}

%inline %{
    TString CanonizeUrlForSaasDeprecatedLegacy(TString url) {
        return NTurbo::CanonizeUrlForSaasDeprecatedLegacy(url);
    }
%}
