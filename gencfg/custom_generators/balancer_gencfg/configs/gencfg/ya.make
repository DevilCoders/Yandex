PY2_LIBRARY()

OWNER(
    g:balancer
)

PY_SRCS(
    gencfg.py
    sections/callisto.py
    sections/clusterstate.py
    sections/gencfg.py
    sections/helpers.py
    sections/other.py
    sections/__init__.py
)

END()
