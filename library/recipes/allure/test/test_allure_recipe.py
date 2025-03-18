import os

import test.tests.common as tests_common
import yatest.common


class TestAllureReport(tests_common.YaTest):

    def test_run(self):
        res = self.run_ya_make_test(
            cwd=yatest.common.source_path("library/recipes/allure/example/pytest"),
            args=["-A", "--test-type", "pytest"], tags=None,
            use_ya_bin=False, use_ymake_bin=False
        )
        assert res.get_tests_count() == 1
        assert res.get_suites_count() == 1

        def verify_links(links):
            assert "allure" in links

        res.verify_test("pytest", "sole chunk", "OK", links=verify_links)
        expected_paths = [
            "allure_report/index.html",
        ]
        for p in expected_paths:
            assert os.path.exists(os.path.join(
                res.output_root, "library/recipes/allure/example/pytest",
                "test-results", "pytest", "testing_out_stuff", p
            ))
