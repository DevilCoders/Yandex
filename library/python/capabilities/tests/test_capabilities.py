import os
import pytest

from library.python.capabilities import capabilities


INVALID_CAP_VALUE = 200


@pytest.mark.skipif(os.getuid() != 0, reason='assuming that we cannot test drop bound without root')
@pytest.mark.parametrize('bound', range(capabilities.cap_last_cap))
def test_drop_bound(bound):
    caps = capabilities.Capabilities()
    caps.drop_bound(bound)


@pytest.mark.skipif(os.getuid() != 0, reason='assuming that we cannot test drop bound without root')
def test_drop_invalid_bound():
    caps = capabilities.Capabilities()
    with pytest.raises(OSError):
        caps.drop_bound(INVALID_CAP_VALUE)


def test_supported():
    caps = capabilities.Capabilities()
    assert caps.is_supported(capabilities.cap_chown)
    assert not caps.is_supported(INVALID_CAP_VALUE)


def test_cap_properties():
    caps = capabilities.Capabilities.from_text(b"cap_setgid,cap_dac_override+ep")

    items = (
        capabilities.cap_setgid,
        capabilities.cap_dac_override,
    )

    for cap in items:
        assert caps.is_set(cap, capabilities.Effective)
        assert caps.is_set(cap, capabilities.Permitted)
        assert not caps.is_set(cap, capabilities.Inheritable)

        caps.set(capabilities.Inheritable, cap)

    for cap in items:
        assert caps.is_set(cap, capabilities.Effective)
        assert caps.is_set(cap, capabilities.Permitted)
        assert caps.is_set(cap, capabilities.Inheritable)

        caps.unset(capabilities.Effective, cap)

    for cap in items:
        assert not caps.is_set(cap, capabilities.Effective)
        assert caps.is_set(cap, capabilities.Permitted)
        assert caps.is_set(cap, capabilities.Inheritable)

    caps.clear()

    for cap in items:
        assert not caps.is_set(cap, capabilities.Effective)
        assert not caps.is_set(cap, capabilities.Permitted)
        assert not caps.is_set(cap, capabilities.Inheritable)
