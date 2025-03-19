"""Tests placeholder."""
import idm_service


def test_importable():
    """Test idm_service is importable."""
    assert idm_service.create_app() is not None
