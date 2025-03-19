# This script creates .apply file, which signals to contrail-dns-reload.service that config needs to be applied.

with open('/etc/contrail/dns/.apply', 'w') as f:
    pass
