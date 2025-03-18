import yatest.common

import test.tests.common as tests_common


class TestPytestRunner(tests_common.YaTest):

    def test(self):
        res = self.run_ya_make_test(
            cwd=yatest.common.source_path("library/python/testing/pytest_runner/test/data"),
            args=["-A"],
        )
        res.verify_test("suite_error.py::devtools::dummy_arcadia::pytests-samples", "suite_error", "FAILED", "REGULAR", comment_contains='raise Exception("error")')
        res.verify_test("test1.py", "test1", "OK")
        res.verify_test("test1.py", "test2", "FAILED", "REGULAR", comment_contains="assert 1 == 2")
        res.verify_test("test1.py", "test_skipped", "SKIPPED", comment_contains="example of a skipped test")
        res.verify_test("test1.py", "test_xfail", "SKIPPED", comment_contains="expected test failure")
        res.verify_test("test1.py", "test_xpass", "FAILED", "REGULAR", comment_contains="[XPASS(strict)]")
        res.verify_test("test2.py::TestClass", "test_method", "FAILED", "REGULAR", comment_contains="raise Exception(\"fail\")")
