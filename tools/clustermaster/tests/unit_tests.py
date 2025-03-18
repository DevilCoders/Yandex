#!/skynet/bin/python

from yatest.common import canonical_execute, binary_path

def test_unit_tests():
    return canonical_execute(binary_path("tools/clustermaster/ut/tools-clustermaster-ut"), file_name="tools-clustermaster-ut")
