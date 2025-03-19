"""Tests for bitbucket repo synced with arcadia"""
from pathlib import Path
import yc_common


def test_ya_make_src_actual():
    """Check all python package files are listed in PY_SRCS section of ya.make"""
    yc_common_source_root = Path(yc_common.__file__).parent
    yc_common_root = yc_common_source_root.parent
    py_srcs = set()
    py_srcs_excluded = {'TOP_LEVEL'}  # ya.make can contain TOP_LEVEL string that is not a module file
    with (yc_common_root / 'ya.make').open() as ya_make_data:
        for line in ya_make_data:
            if line.strip() == 'PY_SRCS(':
                break
        for line in ya_make_data:
            if line.strip() == ')':
                break
            else:
                py_srcs.add(line.strip())

    package_src = set(str(p.relative_to(yc_common_root)) for p in yc_common_source_root.glob('**/*.py'))

    assert package_src == (py_srcs - py_srcs_excluded)
