UNION()

OWNER(g:antiadblock)

BUNDLE(
    antiadblock/configs_api/bin NAME configs_api_bin
)

FILES(
    plugin/__init__.py
)

END()

RECURSE(
    plugin
)
