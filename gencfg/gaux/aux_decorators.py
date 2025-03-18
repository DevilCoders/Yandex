import traceback
import urllib2
import gc
import time


def loadfromfile(f):
    """
        Decorator to extract text data from first arugment like '#filename'. Specific for argparse_types.py
    """

    def tmp(*args, **kwargs):
        if isinstance(args[0], str):
            if args[0].startswith("#http://") or args[0].startswith("#https://"):
                return f(urllib2.urlopen(args[0][1:], timeout=1).read())
            elif args[0].startswith("#"):
                return f(open(args[0][1:]).read().strip())
            else:
                return f(*args, **kwargs)
        else:
            return f(*args, **kwargs)

    return tmp


def static_var(varname, value):
    """
        Static var decorator. Used to save per-function params.
        FIXME: broken in multithread?
    """

    def decorate(func):
        setattr(func, varname, value)
        return func

    return decorate


class Singleton(object):
    """
        Singleton pattern. Nonthread safe.
    """

    def __init__(self, decorated):
        self._decorated = decorated

    def instance(self):
        try:
            return self._instance
        except AttributeError:
            self._instance = self._decorated()
            return self._instance

    def __call__(self):
        raise TypeError('You should use <instance> method')

    def __instancecheck__(self, inst):
        return isinstance(inst, self._decorated)


def memoize(f):
    """ Memoization decorator for functions taking one or more arguments. """

    class memodict(dict):
        def __init__(self, f):
            self.f = f

        def __call__(self, *args):
            return self[args]

        def __missing__(self, key):
            ret = self[key] = self.f(*key)
            return ret

    return memodict(f)


def decayed_memoize(cache_time):
    """
        Memoization decorator for functions taking one or more arguments.
        Memoized values decayed after cache_time
    """

    def real_decorator(f):
        class memodict(dict):
            def __init__(self, f):
                self.f = f
                self.cache_time = cache_time

            def __call__(self, *args):
                cached_at, retval = self[args]
                if time.time() - cached_at > self.cache_time:
                    self[args] = time.time(), self.f(*args)
                return self[args][1]

            def __missing__(self, key):
                ret = self[key] = (time.time(), self.f(*key))
                return ret

        return memodict(f)
    return real_decorator


def hide_value_error(f):
    """
        ArumgentParser catches ValueError and replace it by SystemExit
    """

    def tmp(*args, **kwargs):
        try:
            return f(*args, **kwargs)
        except (ValueError, TypeError):
            raise Exception(
                """\n=========================================\n%s\n================================""" % traceback.format_exc())

    return tmp


def gcdisable(f):
    """
        Disable gc during underlying function execution (sometimes it saves significant time)
    """

    def tmp(*args, **kwargs):
        try:
            gc.disable()
            return f(*args, **kwargs)
        finally:
            gc.enable()

    return tmp
