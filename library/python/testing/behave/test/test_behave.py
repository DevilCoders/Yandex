# coding: utf-8
import os
import test.tests.common as tests_common
import yatest.common


tests_dir = yatest.common.source_path("devtools/dummy_arcadia/test/behave")


class TestBehaveIntegration(tests_common.YaTest):

    def test_run(self):
        def get_execution_count(res):
            count_file = os.path.join(res.output_root, "devtools/dummy_arcadia/test/behave/test-results/pytest/testing_out_stuff/count.txt")
            with open(count_file) as f:
                return int(f.read().strip())

        res = self.run_ya_make_test(cwd=tests_dir, use_ya_bin=False, use_ymake_bin=False)
        assert res.get_tests_count() == 4
        assert res.get_suites_count() == 1

        res.verify_test("features.example.Showing off behave", "Run a simple test", "OK")
        res.verify_test("features.example.Showing off behave", "Run a simple test number two", "OK")
        res.verify_test("features.inner.example.Showing off behave", "Run a simple test", "FAILED", "REGULAR", comment_contains="assert number == 5")
        res.verify_test("features.example.Showing off behave", "Run a simple test number three", "FAILED", "REGULAR", comment_contains="sorry")
        assert get_execution_count(res) == 3

        res = self.run_ya_make_test(cwd=tests_dir, use_ya_bin=False, use_ymake_bin=False, args=["-F", "features.example"])
        assert get_execution_count(res) == 2

        res = self.run_ya_make_test(cwd=tests_dir, use_ya_bin=False, use_ymake_bin=False, args=["-F", "features.inner.example"])
        assert res.get_tests_count() == 1
        assert res.get_suites_count() == 1
        res.verify_test("features.inner.example.Showing off behave", "Run a simple test", "FAILED", "REGULAR", comment_contains="assert number == 5")
        assert get_execution_count(res) == 1

    def test_list(self):
        res = self.run_ya_make_test(cwd=tests_dir, use_ya_bin=False, use_ymake_bin=False)
        assert "features.example.Showing off behave::Run a simple test" in res.err
        assert "features.example.Showing off behave::Run a simple test number two" in res.err
        assert "features.inner.example.Showing off behave::Run a simple test" in res.err
        assert "Total 1 suite" in res.err
        assert "Total 4 tests" in res.err

        res = self.run_ya_make_test(cwd=tests_dir, use_ya_bin=False, use_ymake_bin=False, args=["-F", "features.inner.example"])
        assert "features.inner.example.Showing off behave::Run a simple test" in res.err
        assert "Total 1 suite" in res.err
        assert "Total 1 test" in res.err
