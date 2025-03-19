import pytest

from agent.telemetry import ProberTelemetrySender


@pytest.mark.parametrize(
    "data,size,expected_len,expected",
    [
        [b"", 0, 0, b""],
        [b"", 100, 0, b""],
        [b"a", 128, 1, b"a"],
        [b"a" * 128, 128, 128, b"a" * 128],
        [b"a" * 256, 128, 128, b"a" * 53 + b"[149 bytes truncated]" + b"a" * 54],
        [b"a" * 178, 99, 99, b"a" * 39 + b"[99 bytes truncated]" + b"a" * 40],
        [b"a" * 128, 10, 128, b"a" * 128],
        [b"a" * 21, 20, 20, b"[21 bytes truncated]"],
    ],
    ids=[
        "empty-with-zero-size",
        "empty-with-big-size",
        "size-of-data-less-than-size",
        "size-of-data-equal-to-size",
        "size-of-data-great-than-size",
        "size-of-truncated-on-the-border",
        "size-less-than-infix_size",
        "size-is-equal-to-infix_size"
    ],
)
def test_trim_in_the_middle(data, size, expected_len, expected):
    result = ProberTelemetrySender.trim_data_in_the_middle(data, size)
    assert len(result) == expected_len
    assert result == expected


def test_trim_in_the_middle_negative_cases():
    # negative size
    with pytest.raises(ValueError):
        ProberTelemetrySender.trim_data_in_the_middle(b"one", -1)
