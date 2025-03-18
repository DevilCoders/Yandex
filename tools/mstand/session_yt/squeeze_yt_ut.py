import datetime as dt
import pytest

import session_squeezer.services as squeezer_services
import yaqutils.time_helpers as utime
import yt.wrapper as yt

from mstand_enums.mstand_online_enums import ServiceEnum
from mstand_structs.squeeze_versions import SqueezeVersions
from mstand_utils.yt_options_struct import (
    TableBounds,
    YtJobOptions,
)
from session_yt.squeeze_yt import (
    SqueezeBackendYT,
    YTBackendLocalFiles,
)


@pytest.fixture
def day():
    return dt.date(2021, 3, 1)


@pytest.fixture
def yesterday(day):
    return day - dt.timedelta(days=1)


@pytest.fixture
def tomorrow(day):
    return day + dt.timedelta(days=1)


@pytest.fixture
def patch(day, yesterday, tomorrow, service, cache_version):
    str_day = utime.format_date(day)
    str_yesterday = utime.format_date(yesterday)
    str_tomorrow = utime.format_date(tomorrow)

    squeeze_version = {
        "_mstand_squeeze_versions": {
            service: cache_version,
            "_common": 2,
        }
    }

    cache_filters = {
        "cache_filters": ["filter_hash"],
    }

    def patch():
        return {
            ServiceEnum.YUID_REQID_TESTID_FILTER: {
                yesterday: yt.yson.to_yson_type(str_yesterday, attributes=cache_filters),
                day: yt.yson.to_yson_type(str_day, attributes=cache_filters),
            },
            service: {
                day: yt.yson.to_yson_type(str_day, attributes=squeeze_version),
                tomorrow: yt.yson.to_yson_type(str_tomorrow, attributes=squeeze_version),
            },
        }
    return patch


@pytest.fixture
def squeeze_backend(monkeypatch, patch):
    backend = SqueezeBackendYT(
        local_files=YTBackendLocalFiles(False, []),
        config={},
        yt_job_options=YtJobOptions(),
        table_bounds=TableBounds(),
        yt_lock_enable=False,
        allow_empty_tables=False,
        sort_threads=1,
        add_acl=False,
    )
    monkeypatch.setattr(backend, "get_cache_squeeze_info", patch)
    return backend


@pytest.mark.parametrize("service, cache_version", [
    (ServiceEnum.WEB_DESKTOP, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP].VERSION),
    (ServiceEnum.WEB_TOUCH, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH].VERSION),
    (ServiceEnum.WEB_DESKTOP_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP_EXTENDED].VERSION),
    (ServiceEnum.WEB_TOUCH_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH_EXTENDED].VERSION),
])
def test_cache_version_equal_squeezer_without_replace(day, service, cache_version, squeeze_backend):
    cache_checker = squeeze_backend.get_cache_checker(
        min_versions=SqueezeVersions(),
        need_replace=False,
    )
    status, target_version = cache_checker(day=day, service=service, filter_hash=None)
    assert status
    assert target_version.service_versions[service] == cache_version


@pytest.mark.parametrize("service, cache_version", [
    (ServiceEnum.WEB_DESKTOP, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP].VERSION - 1),
    (ServiceEnum.WEB_TOUCH, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH].VERSION - 1),
    (ServiceEnum.WEB_DESKTOP_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP_EXTENDED].VERSION - 1),
    (ServiceEnum.WEB_TOUCH_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH_EXTENDED].VERSION - 1),
])
def test_cache_version_less_squeezer_without_replace(day, service, cache_version, squeeze_backend):
    cache_checker = squeeze_backend.get_cache_checker(
        min_versions=SqueezeVersions(),
        need_replace=False,
    )
    status, target_version = cache_checker(day=day, service=service, filter_hash=None)
    assert status
    assert target_version.service_versions[service] == cache_version


@pytest.mark.parametrize("service, cache_version", [
    (ServiceEnum.WEB_DESKTOP, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP].VERSION + 1),
    (ServiceEnum.WEB_TOUCH, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH].VERSION + 1),
    (ServiceEnum.WEB_DESKTOP_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP_EXTENDED].VERSION + 1),
    (ServiceEnum.WEB_TOUCH_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH_EXTENDED].VERSION + 1),
])
def test_cache_version_more_squeezer_without_replace(day, service, cache_version, squeeze_backend):
    cache_checker = squeeze_backend.get_cache_checker(
        min_versions=SqueezeVersions(),
        need_replace=False,
    )
    status, target_version = cache_checker(day=day, service=service, filter_hash=None)
    assert status
    assert target_version.service_versions[service] == cache_version


@pytest.mark.parametrize("service, cache_version", [
    (ServiceEnum.WEB_DESKTOP, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP].VERSION),
    (ServiceEnum.WEB_TOUCH, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH].VERSION),
    (ServiceEnum.WEB_DESKTOP_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP_EXTENDED].VERSION),
    (ServiceEnum.WEB_TOUCH_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH_EXTENDED].VERSION),
])
def test_cache_version_equal_squeezer_with_replace(day, service, cache_version, squeeze_backend):
    cache_checker = squeeze_backend.get_cache_checker(
        min_versions=SqueezeVersions(),
        need_replace=True,
    )
    status, target_version = cache_checker(day=day, service=service, filter_hash=None)
    assert status
    assert target_version.service_versions[service] == cache_version


@pytest.mark.parametrize("service, cache_version", [
    (ServiceEnum.WEB_DESKTOP, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP].VERSION - 1),
    (ServiceEnum.WEB_TOUCH, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH].VERSION - 1),
    (ServiceEnum.WEB_DESKTOP_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP_EXTENDED].VERSION - 1),
    (ServiceEnum.WEB_TOUCH_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH_EXTENDED].VERSION - 1),
])
def test_cache_version_less_squeezer_with_replace(day, service, cache_version, squeeze_backend):
    cache_checker = squeeze_backend.get_cache_checker(
        min_versions=SqueezeVersions(),
        need_replace=True,
    )
    status, message = cache_checker(day=day, service=service, filter_hash=None)
    assert not status
    assert message == "Versions is bad: min_version=0, squeeze_version={}, cache_version={}".format(
        cache_version + 1, cache_version,
    )


@pytest.mark.parametrize("service, cache_version", [
    (ServiceEnum.WEB_DESKTOP, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP].VERSION + 1),
    (ServiceEnum.WEB_TOUCH, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH].VERSION + 1),
    (ServiceEnum.WEB_DESKTOP_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP_EXTENDED].VERSION + 1),
    (ServiceEnum.WEB_TOUCH_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH_EXTENDED].VERSION + 1),
])
def test_cache_version_more_squeezer_with_replace(day, service, cache_version, squeeze_backend):
    cache_checker = squeeze_backend.get_cache_checker(
        min_versions=SqueezeVersions(),
        need_replace=True,
    )
    status, target_version = cache_checker(day=day, service=service, filter_hash=None)
    assert status
    assert target_version.service_versions[service] == cache_version


@pytest.mark.parametrize("service, cache_version", [
    (ServiceEnum.WEB_DESKTOP, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP].VERSION),
    (ServiceEnum.WEB_TOUCH, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH].VERSION),
    (ServiceEnum.WEB_DESKTOP_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP_EXTENDED].VERSION),
    (ServiceEnum.WEB_TOUCH_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH_EXTENDED].VERSION),
])
def test_cache_not_exists(yesterday, tomorrow, service, squeeze_backend):
    cache_checker = squeeze_backend.get_cache_checker(
        min_versions=SqueezeVersions(),
        need_replace=False,
    )

    status, message = cache_checker(day=yesterday, service=service, filter_hash=None)
    assert not status
    assert message == "Cache tables don't exist: yuid_data=True, cache_data=False"

    status, message = cache_checker(day=tomorrow, service=service, filter_hash=None)
    assert not status
    assert message == "Cache tables don't exist: yuid_data=False, cache_data=True"


@pytest.mark.parametrize("service, cache_version", [
    (ServiceEnum.WEB_DESKTOP, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP].VERSION),
    (ServiceEnum.WEB_TOUCH, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH].VERSION),
    (ServiceEnum.WEB_DESKTOP_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP_EXTENDED].VERSION),
    (ServiceEnum.WEB_TOUCH_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH_EXTENDED].VERSION),
])
def test_dependence_on_filter_hash(day, cache_version, service, squeeze_backend):
    cache_checker = squeeze_backend.get_cache_checker(
        min_versions=SqueezeVersions(),
        need_replace=False,
    )

    status, target_version = cache_checker(day=day, service=service, filter_hash="filter_hash")
    assert status
    assert target_version.service_versions[service] == cache_version

    status, message = cache_checker(day=day, service=service, filter_hash="unknown")
    assert not status
    assert message == "filter_hash unknown doesn't found in the table attribute"


@pytest.mark.parametrize("service, cache_version", [
    (ServiceEnum.WEB_DESKTOP, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP].VERSION),
    (ServiceEnum.WEB_TOUCH, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH].VERSION),
    (ServiceEnum.WEB_DESKTOP_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_DESKTOP_EXTENDED].VERSION),
    (ServiceEnum.WEB_TOUCH_EXTENDED, squeezer_services.SQUEEZERS[ServiceEnum.WEB_TOUCH_EXTENDED].VERSION),
])
def test_min_version_more_cache_version(day, service, cache_version, squeeze_backend):
    min_version = SqueezeVersions(
        service_versions={
            service: cache_version + 1,
        },
        common=2,
    )

    cache_checker = squeeze_backend.get_cache_checker(
        min_versions=min_version,
        need_replace=False,
    )
    status, message = cache_checker(day=day, service=service, filter_hash=None)
    assert not status
    assert message == "Versions is bad: min_version={}, squeeze_version={}, cache_version={}".format(
        cache_version + 1,  cache_version, cache_version,
    )

    cache_checker = squeeze_backend.get_cache_checker(
        min_versions=min_version,
        need_replace=True,
    )
    status, message = cache_checker(day=day, service=service, filter_hash=None)
    assert not status
    assert message == "Versions is bad: min_version={}, squeeze_version={}, cache_version={}".format(
        cache_version + 1,  cache_version, cache_version,
    )
