{% set cluster = pillar.get('cluster') %}

include:
  - templates.yasmagent
  - units.iface-ip-conf
  - units.clickhouse
  - units.shelves-check
  - units.ya-netconfig


{%- set mount_paths = [] %}
{%- for path, opts in salt['mount.active']().items() if opts['fstype'] in ['ext4'] %}
    {%- do mount_paths.append(path) %}
{%- endfor %}
/usr/local/yasmagent/CONF/agent.mdscommon.conf:
  file.managed:
    - user: monitor
    - group: monitor
    - mode: 644
    - watch_in:
      - service: restart-yasmagent
    - template: jinja
    - contents: |
        [tagkeys]
        actual = ctype, prj, geo, tier, cgroup, common

        [sources]
        common = common_marker
        loadavg = common.loadavg
        iostat = common.iostat
        netstat = common.netstat
        netstat6 = common.netstat6
        vmstat = common.vmstat
        cpu = common.cpu
        disk = common.disk
        mem = common.mem
        instances = common.instances
        push = common.push
        sockstat = common.sockstat
        fdstat = common.fdstat
        cgroup = common.cgroup
        hbf_stats = unistat
        hbf6 = unistat
        hbf4 = unistat

        [options_hbf_stats]
        port=9876
        url=/unistat/

        [options_vmstat]
        cmd = vmstat

        [options_netstat]
        cmd_linux = cat /proc/net/dev
        cmd_freebsd = netstat -i -bdWn

        [options_hbf4]
        port=9876
        url=/dropcount4/

        [options_hbf6]
        port=9876
        url=/dropcount6/

        [options_disk]
        monitored = {{ mount_paths | sort | join(', ') }}
