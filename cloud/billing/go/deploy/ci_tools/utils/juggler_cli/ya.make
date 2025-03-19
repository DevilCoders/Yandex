PY3_PROGRAM(juggler_cli)

OWNER(g:cloud-billing)

PY_MAIN(cloud.billing.go.deploy.ci_tools.utils.juggler_cli.cli)

PY_SRCS(
    __init__.py
    cli.py
)

PEERDIR(juggler/sdk/juggler_sdk)

END()
