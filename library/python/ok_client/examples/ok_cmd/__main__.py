import click
import os
import getpass
import logging

from library.python.ok_client import OkClient, CreateApprovementRequest, StageSimple


TOKEN_ENV_KEY = "OK_TOKEN"


logging.basicConfig(level=logging.WARNING)


def get_token():
    token = os.environ.get(TOKEN_ENV_KEY)

    if not token:
        click.echo(f"No OK token can be found in env {TOKEN_ENV_KEY}. Please provide one")
        exit(1)

    return token


@click.group()
def main():
    pass


@main.command()
@click.argument("ticket")
@click.argument("approvers", nargs=-1)
@click.option("--text", default="Approvement required")
@click.option("--author", default=None)
def create(ticket, approvers, text, author):
    """
    Create an approvement

    :param ticket: Startrek ticket key

    :param approvers: a list of approvers

    :param text: (optional) approvement text/description

    :param author: (optional) approvement author
    """

    ok = OkClient(get_token())

    uuid = ok.create_approvement(payload=CreateApprovementRequest(
        type="tracker",
        object_id=ticket,
        text=text,
        author=author or getpass.getuser(),
        groups=[],
        is_parallel=True,
        stages=[StageSimple(approver=approver) for approver in approvers],
    ))

    click.echo(f"Approvement created. UUID: {uuid}")
    click.echo(
        f'Please post the following comment to {ticket}:\n'
        f'"{{{{iframe '
        f' src="{ok.get_embed_url(uuid)}"'
        f' frameborder=0'
        f' width=100%'
        f' height=400px'
        f' scrolling=no'
        f'}}}}"'
    )


@main.command()
@click.argument("ok_uuid")
def info(ok_uuid):
    """Get approvement info"""

    ok = OkClient(get_token())

    info = ok.get_approvement(uuid=ok_uuid)

    click.echo(
        f"UUID: {ok_uuid}\n"
        f"object_id: {info['object_id']}\n"
        f"status: {info['status']}\n"
        f"resolution: {info['resolution']}"
    )


if __name__ == "__main__":
    main()
