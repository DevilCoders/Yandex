import library.python.awssdk_async_extensions.lib.core as core

import tvm2
try:
    from ticket_parser2_py3.api.v1 import BlackboxClientId
except ImportError:
    from ticket_parser2.api.v1 import BlackboxClientId


async def tvm2_session(self_client_id, self_secret, dst, access_key=None, refresh_hours=1, **kwargs):
    """
    AWS compatible implementation of TVM2 authentification
    https://wiki.yandex-team.ru/passport/tvm2/
    This function uses tvm2 client from library/python/tvm2. Hence you should set env variable TVM2_ASYNC.
    See https://a.yandex-team.ru/arc/trunk/arcadia/library/python/tvm2/README.md for details.
    You can use tvm2_session from core in order to customize tvm client.

    Args:
        self_client_id: TVM2 ID of your client application
            https://abc.yandex-team.ru/resources/?supplier=14&type=47&state=requested&state=approved&state=granted
        self_secret: Your application secret
            https://abc.yandex-team.ru/resources/?supplier=14&type=47&state=requested&state=approved&state=granted
        dst: TVM2 ID of destination application (S3, SQS, etc.)
        access_key: For SQS - your account id in SQS; for S3 - should be empty
        refresh_hours: Interval at which to refresh TVM2 ticket. One hour by default as recommended by TVM2:
            https://wiki.yandex-team.ru/passport/tvm2/theory/
        kwargs: Additional arguments to be provided to aioboto3 Session
    Returns:
        aioboto3 Session object which can be used to get S3, SQS, etc. clients
    Raises:
        core.AioBoto3APIError: In case aioboto3/aiobotocore API has changed and is no longer supporting
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

    client = tvm2.TVM2(
        client_id=self_client_id,
        secret=self_secret,
        blackbox_client=BlackboxClientId.Prod,
        destinations=(dst, ),
    )

    async def get_service_ticket():
        rsp = await client.aio_get_service_tickets(dst)
        return rsp[dst]

    return await core.tvm2_session(get_service_ticket, self_client_id, access_key, refresh_hours, **kwargs)
