import pytest
import session_squeezer.services as squeezer_services

from mstand_enums.mstand_online_enums import ServiceSourceEnum, ServiceEnum


def check_versions(services, history=False):
    extended_services = ServiceEnum.extend_services(services)
    squeezer_services.assert_services(extended_services)
    versions = squeezer_services.get_squeezer_versions(extended_services)
    assert versions.common > 0
    if history:
        assert versions.history > 0
    assert set(extended_services) == set(versions.service_versions.keys())
    assert all(v > 0 for v in versions.service_versions.values())


# noinspection PyClassHasNoInit
class TestServiceEnum:
    def test_service_assertion(self):
        squeezer_services.assert_services(["web", "touch"])
        squeezer_services.assert_services(["video", "images"])

        squeezer_services.assert_services(squeezer_services.SQUEEZERS.keys())
        with pytest.raises(AssertionError):
            squeezer_services.assert_services([])
        with pytest.raises(AssertionError):
            squeezer_services.assert_services(["web_"])
        with pytest.raises(AssertionError):
            squeezer_services.assert_services(["web", "images", "abracadabra"])
        with pytest.raises(AssertionError):
            squeezer_services.assert_services(["web-desktop", "web-touch"])

    def test_service_versions(self):
        check_versions(["web"])
        check_versions(["web", "images", "video"])
        check_versions(squeezer_services.SQUEEZERS.keys())
        check_versions(["web", "touch"], history=True)

    def test_aliases(self):
        expected = sorted(["web", "touch"])
        actual = ServiceEnum.convert_aliases(["web-desktop", "web-touch"])
        assert expected == actual

    def test_all_services(self):
        assert ServiceEnum.ALL == set(squeezer_services.SQUEEZERS.keys())
        extended = set(ServiceEnum.extend_services(ServiceEnum.ALIASES.values()))
        assert ServiceEnum.ALL.issuperset(extended)


# noinspection PyClassHasNoInit
class TestServiceSourceEnum:
    def test_all_sources(self):
        assert ServiceSourceEnum.ALL == set(ServiceEnum.SOURCES.values())
        assert ServiceEnum.ALL == set(ServiceEnum.SOURCES.keys())
        assert ServiceEnum.ALL == set(squeezer_services.SQUEEZERS.keys())
