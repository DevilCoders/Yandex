import yatest.common


check_yaml = yatest.common.binary_path("tools/check_yaml/bin/check_yaml")
data_path = yatest.common.test_source_path("yaml")


def test_valid():
    res = yatest.common.execute([check_yaml, "valid.yaml"], cwd=data_path)
    assert res.exit_code == 0
    assert "Validating valid.yaml: OK" in res.std_out


def test_valid_in_subdir():
    res = yatest.common.execute([check_yaml, "inner_dir/level1/level2/1.yaml"], cwd=data_path)
    assert res.exit_code == 0
    assert "Validating inner_dir/level1/level2/1.yaml: OK" in res.std_out


def test_invalid():
    res = yatest.common.execute([check_yaml, "invalid.yaml"], cwd=data_path, check_exit_code=False)
    assert res.exit_code == 1
    assert "Validating valid.yaml: OK" not in res.std_out
    assert "expected the node content, but found \'<stream end>\'\n  in \"invalid.yaml\", line 1, column 2" in res.std_err


def test_mask():
    res = yatest.common.execute([check_yaml, "*.yaml"], cwd=data_path, check_exit_code=False)
    assert res.exit_code == 1
    assert "Validating valid.yaml: OK" in res.std_out
    assert "expected the node content, but found \'<stream end>\'\n  in \"invalid.yaml\", line 1, column 2" in res.std_err
    assert "inner_dir" not in res.std_out


def test_multiple_masks():
    res = yatest.common.execute([check_yaml, "*.yaml", "inner_dir/*yaml"], cwd=data_path, check_exit_code=False)
    assert res.exit_code == 1
    assert "Validating valid.yaml: OK" in res.std_out
    assert "Validating inner_dir/inner.yaml: OK" in res.std_out
    assert "expected the node content, but found \'<stream end>\'\n  in \"invalid.yaml\", line 1, column 2" in res.std_err


def test_no_matches_found():
    res = yatest.common.execute([check_yaml, "123"], cwd=data_path, check_exit_code=False)
    assert res.exit_code == 1
    assert "Can't find file names that match '123'" in res.std_err


def test_recurse():
    res = yatest.common.execute([check_yaml, "--recurse", "*.yaml"], cwd=data_path, check_exit_code=False)
    assert res.exit_code == 1
    assert "Validating valid.yaml: OK" in res.std_out
    assert "Validating inner_dir/inner.yaml: OK" in res.std_out
    assert "Can't find file names that match '*.yaml' in inner_dir/level1" in res.std_err


def test_recurse_with_subdir_mask():
    res = yatest.common.execute([check_yaml, "--recurse", "*.yaml", "yaml/*.yaml"], cwd=data_path, check_exit_code=False)
    assert res.exit_code == 1
    assert "--recurse and masks with subdirectories are incompatible" in res.std_err


def test_weak_match():
    res = yatest.common.execute([check_yaml, "--recurse", "--weak-match", "*.yaml"], cwd=data_path, check_exit_code=False)
    assert res.exit_code == 1
    assert "Validating valid.yaml: OK" in res.std_out
    assert "Validating inner_dir/inner.yaml: OK" in res.std_out
    assert "Validating inner_dir/level1/level2/1.yaml: OK" in res.std_out
    assert "Can't find file names that match 'inner_dir/level1/*.yaml'" not in res.std_err
