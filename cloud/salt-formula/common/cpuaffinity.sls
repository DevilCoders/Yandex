{% set hostname = grains['nodename'] %}
{% set host_roles = grains['cluster_map']['hosts'][hostname]['roles'] %}

{% if 'seed' in host_roles %}
{% set cores = range(0, grains["num_cpus"]) %}

/etc/systemd/system.conf:
  file.line:
    - match: "CPUAffinity=.*"
    - content: "CPUAffinity={{ cores|join(",") }}"
    - mode: replace

service.systemctl_reload:
  module.run:
    - onchanges:
      - file: /etc/systemd/system.conf
{% endif%}
