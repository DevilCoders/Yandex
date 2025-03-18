import yatest.common
import test.tests.common as tests_common

tests_dir = yatest.common.test_source_path("pytest")


class TestGTest(tests_common.YaTest):

    def test_launch(self):
        res = self.run_ya_make_test(cwd=tests_dir)
        assert res.get_suites_count() == 1
        assert res.get_tests_count() == 3
        res.verify_test("TestSimple", "OK", "OK")

        def verify_comment(comment):
            assert "library/python/testing/gtest/test/gtest/test.cpp:9" in comment
            assert "Expected equality of these values:\n  1\n  2" in comment
            assert "library/python/testing/gtest/test/gtest/test.cpp:10" in comment
            assert "Expected equality of these values:\n  1\n  3" in comment

        res.verify_test("TestSimple", "Failure", "FAILED", "REGULAR", comment=verify_comment)
        res.verify_test("TestSimple", "DISABLED_Test", "SKIPPED")
