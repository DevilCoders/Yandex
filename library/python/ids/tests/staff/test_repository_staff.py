import pytest

from ids.registry import registry

TOKEN = 'sometoken'


@pytest.fixture
def persons():
    return registry.get_repository('staff', 'person', user_agent='test', oauth_token=TOKEN)


@pytest.mark.integration
def test_get_one_person(persons):
    result = persons.get_one(lookup={})
    assert result['id'] is not None


@pytest.mark.integration
def test_get_persons_nopage(persons):
    result = persons.get_nopage(lookup={'_limit': 10})
    assert len(result) == 10
    assert result[0]['id'] is not None

