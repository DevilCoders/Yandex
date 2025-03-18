import sys
import types


class FakeModule(types.ModuleType):
    class Capabilities(object):
        def raise_error(self, *args, **kwargs):
            raise RuntimeError("Capabilities aren't supported on this OS")

        @staticmethod
        def raise_error_static(*args, **kwargs):
            raise RuntimeError("Capabilities aren't supported on this OS")

        is_set = is_supported = set = unset = clear = set_current_proc = raise_error
        from_text = name_for = drop_bound = raise_error_static

    def __getattr__(self, arg):
        return -1


sys.modules[__name__] = FakeModule(__name__)
