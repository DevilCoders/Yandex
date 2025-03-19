"""
Tests for version module.
"""

from dbaas_internal_api.utils.dataproc_joblog.api import filter_necessary_parts

KiB = 2**10
MiB = 2**20
EXAMPLE = [{'Key': '1', 'Size': 4 * KiB}, {'Key': '2', 'Size': 1222}]


class TestDataprocJobLog:
    """
    Test method, that filter necessary parts of log for return
    """

    def test_empty_page_size(self):
        assert filter_necessary_parts(EXAMPLE, 0, 0) == ([], 0)

    def test_full_output(self):
        assert filter_necessary_parts(EXAMPLE, 0, MiB) == (EXAMPLE, 4 * KiB + 1222)

    def test_seeked_output(self):
        assert filter_necessary_parts(EXAMPLE, KiB, MiB) == (
            [{'Key': '1', 'Size': 4 * KiB, 'Seek': KiB}, {'Key': '2', 'Size': 1222}],
            4 * KiB + 1222,
        )
        assert filter_necessary_parts(EXAMPLE, KiB, 3 * KiB) == ([{'Key': '1', 'Size': 4 * KiB, 'Seek': KiB}], 4 * KiB)

    def test_truncated_output(self):
        assert filter_necessary_parts(EXAMPLE, 0, KiB) == ([{'Key': '1', 'Size': 4 * KiB, 'Amount': KiB}], KiB)
        assert filter_necessary_parts(EXAMPLE, 0, 5 * KiB) == (
            [{'Key': '1', 'Size': 4 * KiB}, {'Key': '2', 'Size': 1222, 'Amount': KiB}],
            5 * KiB,
        )

    def test_seeked_and_truncated_output(self):
        assert filter_necessary_parts(EXAMPLE, KiB, 3 * KiB) == ([{'Key': '1', 'Size': 4 * KiB, 'Seek': KiB}], 4 * KiB)
        assert filter_necessary_parts(EXAMPLE, KiB, 4 * KiB) == (
            [{'Key': '1', 'Size': 4 * KiB, 'Seek': KiB}, {'Key': '2', 'Size': 1222, 'Amount': KiB}],
            5 * KiB,
        )

    def test_pagination(self):
        assert filter_necessary_parts(EXAMPLE, 0 * KiB, KiB) == ([{'Key': '1', 'Size': 4 * KiB, 'Amount': KiB}], KiB)
        assert filter_necessary_parts(EXAMPLE, 1 * KiB, KiB) == (
            [{'Key': '1', 'Size': 4 * KiB, 'Seek': KiB, 'Amount': KiB}],
            2 * KiB,
        )
        assert filter_necessary_parts(EXAMPLE, 2 * KiB, KiB) == (
            [{'Key': '1', 'Size': 4 * KiB, 'Seek': 2 * KiB, 'Amount': KiB}],
            3 * KiB,
        )
        assert filter_necessary_parts(EXAMPLE, 3 * KiB, KiB) == (
            [{'Key': '1', 'Size': 4 * KiB, 'Seek': 3 * KiB}],
            4 * KiB,
        )
        assert filter_necessary_parts(EXAMPLE, 4 * KiB, KiB) == ([{'Key': '2', 'Size': 1222, 'Amount': KiB}], 5 * KiB)
        assert filter_necessary_parts(EXAMPLE, 5 * KiB, KiB) == (
            [{'Key': '2', 'Size': 1222, 'Seek': KiB}],
            4 * KiB + 1222,
        )
        assert filter_necessary_parts(EXAMPLE, 6 * KiB, KiB) == ([], 4 * KiB + 1222)
