PY3_LIBRARY()

OWNER(
    g:tools-python
    cracker
)

PEERDIR(
    tools/releaser/src/lib

    contrib/python/click
    contrib/python/dateutil
    contrib/python/hjson
    contrib/python/PyYAML
    contrib/python/requests
    contrib/python/sh
    contrib/python/setuptools
    library/python/oauth
    library/python/gitchronicler
    library/python/vault_client
    infra/nanny/nanny_rpc_client
    infra/awacs/proto
)

PY_SRCS(
   __init__.py
   conf.py
   cli/__init__.py
   cli/arguments.py
   cli/main.py
   cli/options.py
   cli/utils.py
   cli/commands/__init__.py
   cli/commands/changelog.py
   cli/commands/deploy.py
   cli/commands/docker.py
   cli/commands/extra.py
   cli/commands/system.py
   cli/commands/vcs.py
   cli/commands/version.py
   cli/commands/workflow.py
   cli/commands/impl/__init__.py
   cli/commands/impl/ydeploy.py
   cli/commands/impl/qloud.py
)

END()
