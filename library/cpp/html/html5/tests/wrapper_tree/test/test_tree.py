import os
import pytest

import yatest.common

data_path = yatest.common.data_path("indexer_tests_data/tree")
binary = "library/cpp/html/html5/tests/wrapper_tree/printtree"


@pytest.mark.parametrize("input_file", [f for f in os.listdir(data_path) if f != "README.md"])
def test(input_file):
    return yatest.common.canonical_execute(yatest.common.binary_path(binary), [os.path.join(data_path, input_file)], file_name=input_file)
