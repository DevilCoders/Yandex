import library.python.build_info as build_info
import six


def test_simple():
    assert build_info.build_type()
    assert isinstance(build_info.build_type(), str)

    assert build_info.compiler_version()
    assert isinstance(build_info.compiler_version(), str)

    flags = build_info.compiler_flags()
    assert flags
    assert isinstance(flags, str)
    if six.PY3:
        assert '-DUSE_PYTHON2' not in flags, flags
        assert '-DUSE_PYTHON3' in flags, flags
    else:
        assert '-DUSE_PYTHON2' in flags, flags
        assert '-DUSE_PYTHON3' not in flags, flags
        assert '-DARCADIA_PYTHON_UNICODE_SIZE' in flags, flags

    assert build_info.build_info()
    assert isinstance(build_info.build_info(), str)

    # task_id is '0' in local run
    assert build_info.sandbox_task_id()
    assert isinstance(build_info.sandbox_task_id(), str)
    assert build_info.sandbox_task_id() == '0'
