{% for sysctl_file in ['90-cloud-net.conf','90-cloud-kernel.conf','90-cloud-fs.conf'] %}
/etc/sysctl.d/{{ sysctl_file }}:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath }}/{{ sysctl_file }}
    - makedirs: True
  cmd.wait:
    - name: sysctl -p /etc/sysctl.d/{{ sysctl_file }}
    - watch:
      - file: /etc/sysctl.d/{{ sysctl_file }}
{% endfor %}
