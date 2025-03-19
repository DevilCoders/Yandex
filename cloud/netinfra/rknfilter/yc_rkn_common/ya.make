PY3_LIBRARY(yc_rkn_common)


OWNER(g:cloud-netinfra)


PY_SRCS(
    __init__.py
    base.py
    cli_handling.py
    custom_exceptions.py
    local_files.py
    manage_configuration_files.py
    utils.py
    pid_files.py
)


PEERDIR(
    contrib/python/PyYAML
    contrib/python/ConfigArgParse
    contrib/python/boto3
    contrib/python/botocore
    contrib/python/pytz
    contrib/python/pandas
    # contrib/python/json
    # contrib/python/os
)

END()

