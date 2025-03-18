from util.generic.string cimport TString
from libcpp cimport bool as TBool


cdef TString _to_TString(s):
    s = s.encode("utf-8")
    return TString(<const char*>s, len(s))


cdef extern from "antirobot/tools/antiddos/lib/antiddos_tool.h":
    TBool CheckRules(const TString& rules);
    TBool ValidateRequest(const TString& request);
    TBool CheckRequest(const TString& rules, const TString& request);


def check_rules(rules):
    return CheckRules(_to_TString(rules));

def validate_request(request):
    return ValidateRequest(_to_TString(request));

def check_request(rules, request):
    return CheckRequest(_to_TString(rules), _to_TString(request));
