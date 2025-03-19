PY3_LIBRARY(yc_rkn_s3tools)


OWNER(g:cloud-netinfra)


PY_SRCS(
    __init__.py
    s3base.py
    s3connect.py
    s3custom_exceptions.py
    s3delete.py
    s3get.py
    s3put.py
    s3remote_objects.py
)


PEERDIR(
    contrib/python/PyYAML
    contrib/python/ConfigArgParse
    contrib/python/boto3
    contrib/python/botocore
    contrib/python/pytz
    contrib/python/pandas
    contrib/python/requests
    # contrib/python/json
    # contrib/python/os
)

END()

