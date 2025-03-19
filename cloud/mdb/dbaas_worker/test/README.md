## Recipes

### Find your test

```shell
$ ya make -ttL 2>&1 | grep 'bugs'
  test_compute_modify_bugs.py::py3_flake8
  tasks.test_compute_modify_bugs.py::test_scale_down_and_sg_change
```

### Run only your test

```shell
$ ya make -tt -F 'tasks.test_compute_modify_bugs.py'
Number of suites skipped by name: 4, by filter tasks.test_compute_modify_bugs.py

Total 1 suite:
	1 - GOOD
Total 1 test:
	1 - GOOD
Ok
```

[more about tests filtering](https://docs.yandex-team.ru/devtools/test/manual#choose)
