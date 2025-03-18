import datetime

import boto3
import botocore
import pytz
import ticket_parser2.api.v1 as TicketParser2


class Boto3APIError(Exception):
    pass


def tvm2_session(self_client_id, self_secret, dst, access_key=None, refresh_hours=1, **kwargs):
    """
    AWS compatible implementation of TVM2 authentification
    https://wiki.yandex-team.ru/passport/tvm2/

    Args:
        self_client_id: TVM2 ID of your client application
            https://abc.yandex-team.ru/resources/?supplier=14&type=47&state=requested&state=approved&state=granted
        self_secret: Your application secret
            https://abc.yandex-team.ru/resources/?supplier=14&type=47&state=requested&state=approved&state=granted
        dst: TVM2 ID of destination application (S3, SQS, etc.)
        access_key: For SQS - your account id in SQS; for S3 - should be empty
        refresh_hours: Interval at which to refresh TVM2 ticket. One hour by default as recommended by TVM2:
            https://wiki.yandex-team.ru/passport/tvm2/theory/
        kwargs: Additional arguments to be provided to boto3 Session
    Returns:
        boto3 Session object which can be used to get S3, SQS, etc. clients
    Raises:
        Boto3APIError: In case boto3/botocore API has changed and is no longer supporting necessary functionality
    Examples:
        >>> s3_session = tvm2_session(1234567, 'faketvm2secret', 2000273)  # 2000273 - S3 application TVM2 ID
        >>> s3_client = s3_session.client('s3', endpoint_url='http://s3.mds.yandex.net')
        >>> s3_client.list_buckets()

        >>> sqs_session = tvm2_session(1234567, 'faketvm2secret', 2002456,
        ... access_key='your_sqs_access_key')  # 2002456 - SQS application TVM2 ID
        >>> sqs_client = sqs_session.client('sqs', endpoint_url='http://sqs.yandex.net:8771', region_name='eu-west-1')
        >>> sqs_client.list_queues()

    """

    client = TicketParser2.TvmClient(TicketParser2.TvmApiClientSettings(
        self_client_id=self_client_id,
        self_secret=self_secret,
        dsts=[dst],
    ))

    # access_key is only required for SQS
    if access_key is None:
        access_key = 'TVM_V2_{}'.format(self_client_id)

    def refresh():
        ticket = client.get_service_ticket_for(client_id=dst)
        expiry_time = (datetime.datetime.now(pytz.utc) + datetime.timedelta(hours=refresh_hours)).isoformat()
        return dict(
            access_key=access_key,
            secret_key='unused',
            token='TVM2 {}'.format(ticket),
            expiry_time=expiry_time,
        )

    credentials = botocore.credentials.RefreshableCredentials.create_from_metadata(
        metadata=refresh(),
        refresh_using=refresh,
        method='tvm2',
    )
    session = botocore.session.get_session()
    # '_credentials' is not public API so we cannot fully rely on it
    # Need to check and alert early
    if not hasattr(session, '_credentials'):
        raise Boto3APIError("botocore API has changed, session no longer has '_credentials' attribute")
    session._credentials = credentials
    return boto3.Session(botocore_session=session, **kwargs)
