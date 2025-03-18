import pytest

from refsclient import Refs
from refsclient.utils import configure_logging


def test_generics(refs):
    """
    :param Refs refs:
    """
    refs = refs()
    swift = 'swift'

    assert 'Event' in refs.generic_get_types(swift).keys()
    assert 'bics' in refs.generic_get_resources(swift).keys()
    assert 'bic8' in refs.generic_get_type_fields(swift, 'Event').keys()


def test_utils():
    configure_logging()
