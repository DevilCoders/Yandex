PY3_LIBRARY()

OWNER(
    g:tools-python
    cracker
)

PEERDIR(
    tools/releaser/src/lib/deblibs
)

PY_SRCS(
   __init__.py
   changelog_converter.py
   ydeploy.py
   awacs.py
   docker.py
   files.py
   https.py
   nanny.py
   qloud.py
   vcs.py
)

END()
