"""
This module serves as an interactive debug console for the test tools.

Run it is a python script or a py.test to drop into ipython shell
"""


def test_interactive(request):
    import IPython

    class Fx(object):
        def __getattribute__(self, name):
            return request.getfuncargvalue(name)

    fx = Fx()  # noqa

    print '''
==============================================================
    Welcome to py.test shell!
--------------------------------------------------------------
    Only fixtures from common conftest.py are initialized
--------------------------------------------------------------
    fx.X gives fixture X
==============================================================
    '''

    IPython.embed()


if __name__ == '__main__':
    import pytest

    pytest.main(['-s', __file__])
