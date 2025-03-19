{% from slspath + "/map.jinja" import certificates with context %}

certificates_owner:
  user.present:
    - name: {{ certificates.cert_owner }}
    - gid: {{ certificates.cert_group }}
    - require:
      - group: certificates_group
certificates_group:
  group.present:
    - name: {{ certificates.cert_group }}
