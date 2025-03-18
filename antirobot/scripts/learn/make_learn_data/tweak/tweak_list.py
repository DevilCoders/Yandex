def __import__(name, globals=None, locals=None, fromlist=None):
    import sys
    import imp
    # Fast path: see if the module has already been imported.
    try:
        return sys.modules[name]
    except KeyError:
        pass

    # If any of the following calls raises an exception,
    # there's a problem we can't handle -- let the caller handle it.

    fp, pathname, description = imp.find_module(name)

    try:
        return imp.load_module(name, fp, pathname, description)
    finally:
        # Since we may exit via an exception, close fp explicitly.
        if fp:
            fp.close()


class TweakList:
    __tweakClasses = {} # tweak_name => tweak class

    @classmethod
    def Register(cls, tweakCls):
        cls.__tweakClasses[tweakCls.NAME] = tweakCls

    @classmethod
    def GetTweakClass(cls, name):
        return cls.TweakClasses[name]

    @classmethod
    def GetTweakNames(cls):
        return [x for x in cls.__tweakClasses.iterkeys()]

    @classmethod
    def GetTweakClasses(cls):
        return [x for x in cls.__tweakClasses.itervalues()]

def Register(cls):
    TweakList.Register(cls)


#TweakList.Init()
import req_entropy
import spravka_counts
import cookie_req_counts
import hiload
import clicks
import patterns
import frauds
import req_login
import botnets
import bad_subnets
import too_many_redirects
import suspicious
import high_rps
import hidden_image
