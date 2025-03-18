import yatest.common


check_json = yatest.common.binary_path("tools/check_json/bin/check_json")
data_path = yatest.common.test_source_path("json")


def test_valid():
    res = yatest.common.execute([check_json, "valid.json"], cwd=data_path)
    assert res.exit_code == 0
    assert "Validating valid.json: OK" in res.std_out


def test_invalid():
    res = yatest.common.execute([check_json, "invalid.json"], cwd=data_path, check_exit_code=False)
    assert res.exit_code == 1
    assert "Validating valid.json: OK" not in res.std_out
    assert "property name enclosed in double quotes or '}': line 1 column 2 (char 1)" in res.std_err


def test_mask():
    res = yatest.common.execute([check_json, "*.json"], cwd=data_path, check_exit_code=False)
    assert res.exit_code == 1
    assert "Validating valid.json: OK" in res.std_out
    assert "property name enclosed in double quotes or '}': line 1 column 2 (char 1)" in res.std_err


def test_no_matches_found():
    res = yatest.common.execute([check_json, "123"], cwd=data_path, check_exit_code=False)
    assert res.exit_code == 1
    assert "Can't find file names that match '123'" in res.std_err


def test_recurse():
    res = yatest.common.execute([check_json, "--recurse", "*.json"], cwd=data_path, check_exit_code=False)
    assert res.exit_code == 1
    assert "Validating valid.json: OK" in res.std_out
    assert "Validating inner_dir/inner.json: OK" in res.std_out
    assert "Can't find file names that match 'inner_dir/level1/*.json'" in res.std_err


def test_weak_match():
    res = yatest.common.execute([check_json, "--recurse", "--weak-match", "*.json"], cwd=data_path, check_exit_code=False)
    assert res.exit_code == 1
    assert "Validating valid.json: OK" in res.std_out
    assert "Validating inner_dir/inner.json: OK" in res.std_out
    assert "Validating inner_dir/level1/level2/1.json: OK" in res.std_out
    assert "Can't find file names that match 'inner_dir/level1/*.json'" not in res.std_err
