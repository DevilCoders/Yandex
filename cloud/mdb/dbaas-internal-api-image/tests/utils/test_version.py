"""
Tests for version module.
"""

from dbaas_internal_api.utils.version import Version


class TestVersion:
    """
    Tests for Version class.
    """

    def test_load(self):
        version = Version.load('3.6')
        assert version.major == 3
        assert version.minor == 6
        assert version.string == '3.6'

    def test_load_full_version(self):
        version = Version.load('3.6.4.5', strict=False)
        assert version.major == 3
        assert version.minor == 6
        assert version.string == '3.6.4.5'

    def test_eq(self):
        assert Version(major=3, minor=6) == Version(major=3, minor=6)

    def test_lt(self):
        assert Version(major=3, minor=5) < Version(major=3, minor=6)

    def test_le_when_less(self):
        assert Version(major=3, minor=5) <= Version(major=3, minor=6)

    def test_le_when_equal(self):
        assert Version(major=3, minor=5) <= Version(major=3, minor=5)

    def test_eq_major_only(self):
        assert Version(major=3) == Version(major=3)

    def test_eq_minor_zero(self):
        assert Version(major=3, minor=0) != Version(major=3)

    def test_lt_major_only(self):
        assert Version(major=3) < Version(major=4)

    def test_lt_mixed(self):
        assert Version(major=3) < Version(major=3, minor=3)

    def test_lt_when_compare_gt(self):
        assert Version(major=10) > Version(major=9, minor=6)

    def test__str__(self):
        assert str(Version(major=3, minor=4)) == '3.4'

    def test_to_string_with_default_separator(self):
        assert Version(major=3, minor=4).to_string() == '3.4'

    def test_to_string_with_custom_separator(self):
        assert Version(major=3, minor=5).to_string('_') == '3_5'
