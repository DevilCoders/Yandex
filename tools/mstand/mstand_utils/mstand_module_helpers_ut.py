import logging
import os
import sys

import pytest

import mstand_utils.mstand_module_helpers as mstand_umodule

import yaqutils.file_helpers as ufile
import yaqutils.module_helpers as umodule


@pytest.fixture
def data_path(request):
    try:
        import yatest.common
        return str(yatest.common.source_path("tools/mstand/mstand_utils/tests/data"))
    except:
        return str(request.config.rootdir.join("mstand_utils/tests/data"))


TEST_MODULE_NAME = "mstand_import_test_module"


def prepare_temp_module_dir(tmpdir):
    base_dir = str(tmpdir)
    tmp_module_dir = os.path.join(base_dir, TEST_MODULE_NAME)
    os.mkdir(tmp_module_dir)

    sys.path.append(base_dir)
    init_py = os.path.join(tmp_module_dir, "__init__.py")
    ufile.write_text_file(init_py, "# mstand import test\n")
    return tmp_module_dir


# noinspection PyClassHasNoInit
class TestImportObject:
    def test_import_existing_object(self, tmpdir, data_path):
        tmp_module_dir = prepare_temp_module_dir(tmpdir)
        logging.info("tmp_module_dir = %s", tmp_module_dir)

        test_class = mstand_umodule.import_user_object(
            "test_import",
            "TestClassForImportTest",
            source=data_path,
            dest_dir=tmp_module_dir,
            dest_module=TEST_MODULE_NAME
        )
        test_instance = test_class()
        assert (test_instance(100500) == 100500)

    def test_import_single_file(self, tmpdir, data_path):
        tmp_module_dir = prepare_temp_module_dir(tmpdir)

        test_class = mstand_umodule.import_user_object(
            "test_import",
            "TestClassForImportTest",
            source=os.path.join(data_path, "test_import.py"),
            dest_dir=tmp_module_dir,
            dest_module=TEST_MODULE_NAME
        )
        test_instance = test_class()
        assert (test_instance(100500) == 100500)

    def test_import_missing_module(self, tmpdir, data_path):
        tmp_module_dir = prepare_temp_module_dir(tmpdir)

        with pytest.raises(Exception):
            mstand_umodule.import_user_object(
                "non_existing_module",
                "TestClassForImportTest",
                source=data_path,
                dest_dir=tmp_module_dir,
                dest_module=TEST_MODULE_NAME
            )

    def test_import_missing_object(self, tmpdir, data_path):
        tmp_module_dir = prepare_temp_module_dir(tmpdir)

        with pytest.raises(Exception):
            mstand_umodule.import_user_object(
                "test_import",
                "NonExistingClass",
                source=data_path,
                dest_dir=tmp_module_dir,
                dest_module=TEST_MODULE_NAME
            )

    def test_create_user_object_good(self):
        user_kwargs = ['ctor_one=100500', 'ctor_two="two_value"', "ctor_three=null"]
        sign_test_instance = mstand_umodule.create_user_object("mstand_utils.tests.data.test_import", "SignatureSample",
                                                               source=None, kwargs=user_kwargs)
        sign_test_instance.sample_method()

    def test_create_user_object_buggy_module(self):
        with pytest.raises(umodule.UserModuleException):
            mstand_umodule.create_user_object("mstand_utils.buggy_module", "BuggyClassSample",
                                              source=None, kwargs=None)

    def test_create_user_object_bad(self):
        user_kwargs = ['ctor_uno=100500', 'ctor_dos="two_value"', "ctor_tres=null"]
        with pytest.raises(umodule.UserModuleException):
            mstand_umodule.create_user_object("mstand_utils.tests.data.test_import", "SignatureSample",
                                              source=None, kwargs=user_kwargs)
