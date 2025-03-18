import logging
import sys
import warnings
from typing import Optional

import click

from dssclient import version_str, Dss
from dssclient.client import TypeConnectorCompat
from dssclient.exceptions import DssClientException
from dssclient.utils import configure_logging


AUTH_OBJ: TypeConnectorCompat = None
HOST: Optional[str] = None
TIMEOUT: Optional[int] = None
RETRIES: Optional[int] = None
SIGNING_PARAMS = dict(
    sorted([(key, val) for key, val in Dss.signing_params.__dict__.items() if '_' not in key]))


def get_dss() -> Dss:
    return Dss(AUTH_OBJ, host=HOST, timeout=TIMEOUT, retries=RETRIES)


@click.group()
@click.version_option(version=version_str)
@click.option('--verbose', help='Be verbose, output more info.', is_flag=True)
@click.option('--auth', help='Authentication data.')
@click.option('--host', help='Overrides default DSS host.')
@click.option('--timeout', help='Overrides the default request timeout.', type=int)
@click.option('--retries', help='Overrides the default request retry count.', type=int)
def base(verbose, auth, host, timeout, retries):
    """DSS client command line utility."""
    configure_logging(logging.DEBUG if verbose else logging.INFO)

    if auth:
        if ';' in auth:
            auth_obj = {}
            for arg_pair in auth.split(';'):
                arg_pair = arg_pair.split('=')
                auth_obj[arg_pair[0]] = arg_pair[1]

        else:
            auth_obj = auth

        global AUTH_OBJ
        AUTH_OBJ = auth_obj

    if host:
        global HOST
        HOST = host

    if timeout:
        global TIMEOUT
        TIMEOUT = timeout

    if retries:
        global RETRIES
        RETRIES = retries


@base.command()
@click.option('--file', help='File to sign.', type=click.Path(exists=True, dir_okay=False))
@click.option('--string', help='String to sign.')
@click.option('--type', help='Signature type to use.', type=click.Choice(SIGNING_PARAMS.keys()), default='ghost3410')
@click.option('--base64', help='Output base64 encoded signing result.', is_flag=True)
def sign(file, string, type, base64):
    """Sign data from input or file."""

    dss = get_dss()

    if file:
        target = dss.cls_file(file)

    elif string:
        target = string

    else:
        raise DssClientException('Please supply either --file or --string ')

    cls_signing_params = SIGNING_PARAMS[type]

    # Не всегда доступен нулевой умолчательный сертификат, поэтому выбираем первый доступный.
    certificate = dss.certificates.get_all()[0]

    result = dss.documents.sign(target, params=cls_signing_params(certificate))[0]

    click.secho(result.signed_base64 if base64 else result.signed_bytes)


@base.group()
def policies():
    """Various policies related commands group."""


@policies.command()
def signing():
    """Print out signing policy.

    > dssclient --auth "client_id=AAA;username=BBB;password=CCC" policies signing

    """
    click.secho('')
    click.secho(get_dss().policies.get_signing_policy().pformat())


@policies.command()
def users():
    """Print out users management policy.

    > dssclient --auth "client_id=AAA;username=BBB;password=CCC" policies users

    """
    click.secho('')
    click.secho(get_dss().policies.get_users_policy().pformat())


@base.group()
def certificates():
    """Certificates related commands group."""


@certificates.command()
@click.option('--show-pub', help='Output base64 encoded public keys.', is_flag=True)
def list(show_pub):
    """Print out available certificates data.

    > dssclient --auth "client_id=AAA;username=BBB;password=CCC" certificates list

    """
    click.secho('')

    for cert in get_dss().certificates.get_all():
        click.secho(cert.pformat())
        click.secho('')

        if show_pub:
            click.secho(f'    Public key: {cert.public_key_base64}')
            click.secho('')


@base.group()
def certificate_requests():
    """Certificates requests related commands group."""


@certificate_requests.command()
def list():
    """Print out available certificate requests data.

    > dssclient --auth "client_id=AAA;username=BBB;password=CCC" certificate_requests list

    """
    click.secho('')

    for req in get_dss().certificates.requests.get_all():
        click.secho(req.pformat())
        click.secho('')


def main():
    """
    CLI entry point.
    """
    warnings.filterwarnings('ignore')

    try:
        base(obj={})

    except DssClientException as e:
        click.secho(f'{e}', err=True, fg='red')
        sys.exit(1)


if __name__ == '__main__':
    main()
