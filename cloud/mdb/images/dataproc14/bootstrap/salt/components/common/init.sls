{% import 'components/hadoop/macro.sls' as m with context %}

common-packages:
    pkg.installed:
        - refresh: False
        - pkgs:
            - bigtop-utils
            - bigtop-jsvc
            - bind9
            - python-matplotlib

metadata-present-in-/etc/hosts:
    host.present:
        - ip:
            - 169.254.169.254
        - names:
            - metadata.internal

/usr/local/share/ca-certificates/yandex-cloud-ca.crt:
  file.managed:
    - source:
      - https://storage.yandexcloud.net/cloud-certs/CA.pem
      - salt://{{ slspath }}/CA.pem
    - mode: 0644
    - skip_verify: True
  cmd.run:
    - name: 'update-ca-certificates --fresh'
    - onchanges:
      - file: /usr/local/share/ca-certificates/yandex-cloud-ca.crt

# temporary duplication to break dependency of cloud/mdb/dataproc-infra-tests/tests/helpers/pillar.py:232
# remove after new images will become stable
/srv/CA.pem:
  file.managed:
    - source:
      - /usr/local/share/ca-certificates/yandex-cloud-ca.crt
    - mode: 0644
    - skip_verify: True
    - require:
      - file: /usr/local/share/ca-certificates/yandex-cloud-ca.crt

{% if not salt['ydputils.is_presetup']() %}
# This state is needed for removing fqdn from 127.0.0.1 record
# Otherwise, java daemons can start on loopback interface
localhost-present-in-/etc/hosts:
    host.present:
        - ip:
            - 127.0.0.1
        - names:
            - localhost
        - clean: True

# We use custom method for getting ipv4 addresses,
# because we need to filter them and throw loopback interfaces.
{% set ipv4 = salt['ydputils.fqdn_ipv4']() %}
{% if ipv4 | length > 0 %}
fqdn-present-in-/etc/hosts:
    host.present:
        - ip:
{% for ip_address in ipv4 %}
            - {{ ip_address }}
{% endfor %}
        - names:
            - {{ salt['grains.get']('dataproc:fqdn') }}
        - clean: True

/etc/hostname:
  file.managed:
    - contents:
      - {{ salt['grains.get']('dataproc:fqdn') }}
    - user: root
    - group: root
    - mode: '0644'

/etc/dhcp/dhclient.conf:
  file.append:
    - text:
      - 'supersede host-name "{{ salt['grains.get']('dataproc:fqdn') }}";'

{% endif %}
{% endif %}

{%- set extra_dns_records = salt['pillar.get']('data:agent:hosts', {}) -%}
{% for hostname, ip in extra_dns_records.items() %}
{{ hostname }}-present-in-/etc/hosts:
    host.present:
        - ip:
              - {{ ip }}
        - names:
              - {{ hostname }}
        - clean: True
{% endfor %}

/root/.bashrc:
  file.append:
    - text: alias hs='salt-call --out=highstate --state-out=changes --output-diff --log-file-level=info --log-level=quiet state.highstate queue=True'
