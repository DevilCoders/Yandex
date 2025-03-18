import aiobotocore
import library.python.awssdk_async_extensions.lib.core as core
import pytest


def test_session_credentials_attribute():
    session = aiobotocore.session.get_session()
    assert hasattr(session, '_credentials')


@pytest.mark.asyncio
async def test_core_function_works():
    async def get_ticket():
        return '1324'

    await core.tvm2_session(get_ticket, 123)
