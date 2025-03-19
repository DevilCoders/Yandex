import click
import grpc
from prettytable import PrettyTable

from yandex.cloud.priv.quota import quota_pb2
from yandex.cloud.priv.serverless.functions.v1 import \
    quota_service_pb2_grpc as serverless_quota_service


SERVICES = {
    'serverless': serverless_quota_service,
}


def quota_service(ssl_root, endpoint, service, iam_token):
    if service not in SERVICES.keys():
        raise 'No service named {} supported'.format(service)

    with open(ssl_root, 'rb') as cert:
        ssl_creds = grpc.ssl_channel_credentials(cert.read())

    call_creds = grpc.access_token_call_credentials(iam_token)
    chan_creds = grpc.composite_channel_credentials(ssl_creds, call_creds)

    stub = SERVICES[service].QuotaServiceStub
    channel = grpc.secure_channel(endpoint, chan_creds)

    return stub(channel)


@click.group()
@click.option('--ssl-root', default='/etc/ssl/certs/YandexInternalRootCA.pem',
              help='Path to YandexInternalRootCA.pem')
@click.option('--iam-token', help='Cloud IAM Token', required=True)
@click.option('--endpoint', help='Backend endpoint', required=True)
@click.option('--service', help='Service to operate', required=True)
@click.option('--cloud-id', help='Cloud ID to operate', required=True)
@click.pass_context
def cli(ctx, ssl_root, iam_token, endpoint, service, cloud_id):
    ctx.meta['cloud-id'] = cloud_id
    ctx.meta['stub'] = quota_service(ssl_root, endpoint, service, iam_token)


def abort_if_false(ctx, param, value):
    if not value:
        ctx.abort()


@cli.command('list-quota')
@click.pass_context
def list_quota(ctx):
    click.echo(click.style(
        'Listing quotas for cloud {}\n'.format(ctx.meta['cloud-id']),
        fg='green', bold=True,
    ))

    stub = ctx.meta['stub']
    req = quota_pb2.GetQuotaRequest(cloud_id=ctx.meta['cloud-id'])
    resp = stub.Get(req)

    t = PrettyTable()
    t.field_names = ["Name", "Value", "Quota"]
    t.align["Name"] = "l"
    t.align["Value"] = "r"
    t.align["Quota"] = "r"

    for i in resp.metrics:
        t.add_row([i.name, i.value, i.limit])
    print t


@cli.command('set-quota')
@click.option('--name', help='Quota name', required=True)
@click.option('--value', help='Quota value', required=True)
@click.option('--yes', is_flag=True, callback=abort_if_false,
              expose_value=False,
              prompt='Are you sure you want to set quota?')
@click.pass_context
def set_quota(ctx, name, value):
    click.echo(click.style(
        'Setting quota {}={} for cloud {}\n'.format(
            name, value, ctx.meta['cloud-id']
        ),
        fg='green', bold=True,
    ))

    stub = ctx.meta['stub']
    req = quota_pb2.UpdateQuotaMetricRequest(
        cloud_id=ctx.meta['cloud-id'],
        metric=quota_pb2.MetricLimit(
            name=name,
            limit=int(value),
        ),
    )
    stub.UpdateMetric(req)
    click.echo(click.style(
        'Set quota {}={} for cloud {}!'.format(
            name, value, ctx.meta['cloud-id']
        ),
        fg='green', bold=True,
    ))


def main():
    cli()
