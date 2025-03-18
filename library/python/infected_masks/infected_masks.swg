%module infected_masks

// Binding of TSafeBrowsingMasks library/cpp/infected_masks.

%include "exception.i"
%include <std_pair.i>
%include <std_string.i>
%include <std_vector.i>
%include <typemaps.i>

%include <bindings/swiglib/defaults.swg>
%include <bindings/swiglib/stroka.swg>

%exception {
  try {
    $action
  } catch (const std::exception& e) {
    SWIG_exception(SWIG_RuntimeError, e.what());
  }
}

%begin %{
    // Let Python define all its symbols first and allow Aracadia to redefine them (the
    // opposite order breaks build).
    #include <Python.h>

    #include <library/python/infected_masks/infected_masks_pywrap.h>
%}
%typemap(out) BaseMatcher* {
    $result = SWIG_NewPointerObj(
        SWIG_as_voidptr($1),
        SWIGTYPE_p_BaseMatcher,
        SWIG_POINTER_OWN | 0
    );
}
%include <library/python/infected_masks/infected_masks_pywrap.h>
%template(MatchItem) std::pair<std::string, MatchData>;
%template(MatchResult) std::vector<MatchItem>;

%pythoncode %{
def __BaseMatcher___setstate__(self, state):
    if not (hasattr(self, 'this') and self.this):
        self.__init__()
    self._setstate(state)
BaseMatcher.__setstate__ = __BaseMatcher___setstate__
%}

%pythoncode %{
import cPickle as pickle

class Matcher(object):
    def __init__(self, base_matcher):
        self._base_matcher = base_matcher

    def IsInfectedUrl(self, url):
        return self._base_matcher.IsInfectedUrl(url)

    def FindAll(self, url):

        def unpickle(data):
            if data:
                return pickle.loads(data)
            return None

        result = self._base_matcher.FindAll(url)
        return [
            (mask, tuple(unpickle(item) for item in data))
            for mask, data in result
        ]


class MatcherMaker(object):
    def __init__(self):
        self._base_maker = BaseMatcherMaker()

    def AddMask(self, mask, data=None):
        data_str = ''
        if data is not None:
            data_str = pickle.dumps(data, protocol=pickle.HIGHEST_PROTOCOL)
        self._base_maker.AddMask(mask, data_str)

    def GetMatcher(self):
        return Matcher(self._base_maker.GetMatcher())
%}
