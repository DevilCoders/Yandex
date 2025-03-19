#!/usr/bin/python3
# flake8: noqa
# pylint: skip-file

from kmip.pie.client import ProxyKmipClient

client = ProxyKmipClient(
    config='client',
    config_file='/etc/pykmip/client.conf',
)

try:
    client.open()
    client.close()
    print("PASSED")
except Exception:
    print("FAIL")
