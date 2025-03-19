{% set hostname = salt['grains.get']('dataproc:fqdn') %}
{% set masternodes = salt['ydputils.get_masternodes']() %}

local-required-dns-records-available:
    dns_record.available:
       - records: [ {{ hostname }} ]

master-required-dns-records-available:
    dns_record.available:
       - records: {{ masternodes | json }}
