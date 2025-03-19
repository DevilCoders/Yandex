cleanup_after_upgrade:
  cmd.run:
    - name: |
        service pgsync restart
        rm -f /root/.ssh/id_rsa
{% set version_from = salt['pillar.get']('version_from', salt['pillar.get']('data:version_from', None)) %}
{% if version_from %}
        rm -rf /tmp/etc-{{ version_from }}-data-bak
        rm -rf /var/lib/postgresql/{{ version_from }}/data
{% endif %}
{% set master_fqdn = salt['pillar.get']('master_fqdn', None) %}
{% if master_fqdn %}
        sed -i '/{{ master_fqdn }}/d' /root/.ssh/authorized_keys2
{% endif %}
