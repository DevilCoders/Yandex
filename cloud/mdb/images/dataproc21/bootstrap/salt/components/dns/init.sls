{% if not salt['ydputils.is_presetup']() %}
/etc/systemd/resolved.conf:
  file.append:
    - text:
      - "# Yandex.Cloud configuration for using internal dns for internal addressing"
      - "[Resolve]"
      - "DNS = {{ salt['ydputils.get_internal_dns_addr']() }}"
      - "Domains = {{ salt['ydputils.get_internal_domains']() }}"

systemd-resolved:
  service.running:
    - enable: true
    - require:
      - file: /etc/systemd/resolved.conf
    - watch:
      - file: /etc/systemd/resolved.conf

metadata-present-in-/etc/hosts:
  host.present:
    - ip:
      - 169.254.169.254
    - names:
      - metadata.internal

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

{% endif %}
