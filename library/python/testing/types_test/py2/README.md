Helper library to add mypy tests to your project (PY2 Edition)
=============================================================

What is *mypy*? *mypy* is a tool to validate type annotations in your Python project.

While it works fine with external world Python, we had to make some magic for it to work in Arcadia,
and this is a library to validate your project easily.

How should I do it?
===================

First thing you should know: *mypy* is slow. Despite best efforts its code is overcomplicated at times and just cannot process every single included Python file in a reasonable time. May be in the future… But now you have to select some subdirectory in Arcadia to check.

Second thing comes from the first: *mypy* is not supported natively in Arcadia yet. You should add tests for your project manually.

Imagine, you have a project **myproject** with the following structure:

    arcadia/
            ya.make
            search/
                   ya.make
                   myproject/
                             ya.make
                             bin/
                                 main.py
                                 ya.make
                             component1/
                                 file1.py
                                 file2.py
                                 ya.make
                                 tests/
                                       test_file1.py
                                       ya.make
                            component2/
                                file1.py
                                file2.py
                                ya.make
                            ...

The best idea would be to check the whole `search/myproject/` tree. However you should know: **PY2_PROGRAM targets cannot be linked wth tests!** That's why smart people move all the logic into PY2_LIBRARY and only call very simple `main()` in PY2_PROGRAM main.py

Difference from Python 3
------------------------
MyPy is developing with python3. We can't using it's api inside python2 code. But mypy binary supports `--py2` flag as argument to test py2 code.
The only problem is to resolve arcadia directories structure while testing. So we decide to use **SFX** Tool for extracting binaries. 

Create test target
------------------

To test the project you should create a new test target, e.g. `search/myproject/tests/mypy/` and put two files there:

### ya.make

```
PY2TEST()

# This include is mandatory, it does all the job
INCLUDE(${ARCADIA_ROOT}/library/python/testing/types_test/py2/typing.inc)

PEERDIR(
    # These targets are needed to include all the checked files and their
    # dependencies to test binary
    search/myproject/component1
    search/myproject/component2
)

TEST_SRCS(
    conftest.py
)

# Since mypy is rather slow it could be a good idea to force test
# to have MEDIUM or even LARGE size, and increase timeout.
SIZE(MEDIUM)
TIMEOUT(600)

END()
```

### conftest.py

```py
from library.python.testing.types_test.py2.config import DefaultMyPyConfig  # optional

# Uses mypy --package instead of --module, don't stop on specified module, check recursively to the end
check_recursive = False

# Skips specified module from checking, avoids searching library stubs for them, returns tp.Any
modules_to_delete = ['modules', 'with', 'mypy', 'syntax', 'errors']

# Optional override default config
class MyPyConfig(DefaultMyPyConfig):
    follow_imports = 'silent'


def mypy_check_module():
    # Specify module path relative to Arcadia root,
    # pointing to subdirectory of your project.

    return 'search.myproject'
```

Now just run `ya make -tt search/myproject/tests/mypy` and look at the results. Voilà!

Learn more about *mypy*: [http://www.mypy-lang.org ](http://www.mypy-lang.org/)

DON'T FORGET ABOUT `# encoding: utf-8` in header of scanning files
