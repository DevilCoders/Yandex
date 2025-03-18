import datetime

import aioboto3
import aiobotocore
import pytz


class AioBoto3APIError(Exception):
    pass


async def tvm2_session(get_service_ticket, self_client_id, access_key=None, refresh_hours=1, **kwargs):
    """
    AWS compatible implementation of TVM2 authentification
    https://wiki.yandex-team.ru/passport/tvm2/

    Args:
        get_service_ticket: async function returning fresh tvm ticket.
        access_key: For SQS - your account id in SQS; for S3 - should be empty
        refresh_hours: Interval at which to refresh TVM2 ticket. One hour by default as recommended by TVM2:
            https://wiki.yandex-team.ru/passport/tvm2/theory/
        kwargs: Additional arguments to be provided to aioboto3 Session
    Returns:
        aioboto3 Session object which can be used to get S3, SQS, etc. clients
    Raises:
        AioBoto3APIError: In case aioboto3/aiobotocore API has changed and is no longer supporting
            necessary functionality
    Examples:
        >>> s3_session = tvm2_session(1234567, 'faketvm2secret', 2000273)  # 2000273 - S3 application TVM2 ID
        >>> s3_client = s3_session.client('s3', endpoint_url='http://s3.mds.yandex.net')
        >>> await s3_client.list_buckets()

        >>> sqs_session = tvm2_session(1234567, 'faketvm2secret', 2002456,
        ... access_key='your_sqs_access_key')  # 2002456 - SQS application TVM2 ID
        >>> sqs_client = sqs_session.client('sqs', endpoint_url='http://sqs.yandex.net:8771', region_name='eu-west-1')
        >>> await sqs_client.list_queues()

    """

    # access_key is only required for SQS
    if access_key is None:
        access_key = 'TVM_V2_{}'.format(self_client_id)

    async def refresh():
        ticket = await get_service_ticket()
        expiry_time = (datetime.datetime.now(pytz.utc) + datetime.timedelta(hours=refresh_hours)).isoformat()
        return dict(
            access_key=access_key,
            secret_key='unused',
            token='TVM2 {}'.format(ticket),
            expiry_time=expiry_time,
        )

    credentials = aiobotocore.credentials.AioRefreshableCredentials.create_from_metadata(
        metadata=await refresh(),
        refresh_using=refresh,
        method='tvm2',
    )
    session = aiobotocore.session.get_session()
    # '_credentials' is not public API so we cannot fully rely on it
    # Need to check and alert early
    if not hasattr(session, '_credentials'):
        raise AioBoto3APIError("aiobotocore API has changed, session no longer has '_credentials' attribute")
    session._credentials = credentials
    return aioboto3.Session(botocore_session=session, **kwargs)
