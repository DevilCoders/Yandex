{% from "components/greenplum/map.jinja" import dep,sysvars with context %}

{% if salt['grains.get']('virtual', 'physical') != 'lxc' and not salt['pillar.get']('data:lxc_used', False)  and salt['grains.get']('virtual_subtype', None) != 'Docker' %}

{{ sysvars.sysctlconf }}:
  file.managed:
    - source: salt://{{slspath}}/conf/sysctl/sysctl.conf
    - template: jinja

/sbin/sysctl -p {{ sysvars.sysctlconf }}:
  cmd.run:
    - onchanges:
      - file: {{ sysvars.sysctlconf }}

Apply_gpdb_related_sysctl_on_boot:
  file.append:
    - name: {{ dep.rc_local }}
    - text:
      - /lib/systemd/systemd-sysctl {{ sysvars.sysctlconf }}
{% endif %}

