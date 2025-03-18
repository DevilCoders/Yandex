# coding: utf-8

import os

import test.tests.common as tests_common
import yatest.common


class TestRunInQemu(tests_common.YaTest):

    def test_run(self):
        res = self.run_ya_make_test(cwd=yatest.common.source_path("library/recipes/qemu_kvm/example"), args=["-A", "--test-type", "pytest"], tags=None)
        assert res.get_tests_count() == 1
        assert res.get_suites_count() == 1

        res.verify_test("test.py", "test_run_in_qemu_works", "OK")
        expected_paths = [
            "qemu_kvm.recipe.start.log",
            "qemu_kvm.recipe.stop.log",
            "rootfs.img_serial.out",
            "test.py.test_run_in_qemu_works.log",
        ]
        for p in expected_paths:
            assert os.path.exists(os.path.join(
                res.output_root, "library/recipes/qemu_kvm/example",
                "test-results", "pytest", "testing_out_stuff", p
            ))
        with open(os.path.join(
                res.output_root, "library/recipes/qemu_kvm/example",
                "test-results", "pytest", "testing_out_stuff", "test.py.test_run_in_qemu_works.log")) as f:
            data = f.read()
            for k in ['bin', 'boot', 'etc', 'lib', 'sys', 'var', 'sbin']:
                p = "test_run_in_qemu_works: qemu_vm: ls /: {}".format(k)
                assert p in data
