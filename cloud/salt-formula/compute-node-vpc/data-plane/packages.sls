/etc/udev/rules.d/vhost-net.rules:
  file.managed:
    - contents: 'KERNEL=="vhost-net", GROUP="kvm", MODE="0660"'

vrouter_net_modules:
  kmod.present:
    - mods:
      - vhost_net
    - persist: True
    - require:
      - file: /etc/udev/rules.d/vhost-net.rules

contrail_vrouter_agent_packages:
  yc_pkg.installed:
    - pkgs:
      - contrail-vrouter-dkms
      - contrail-vrouter-utils
      - contrail-vrouter-agent
      - contrail-vrouter-agent-dbg
    - hold_pinned_pkgs: True
    - require:
      - file: /etc/contrail/contrail-vrouter-agent_log4cplus.properties
      - file: /etc/contrail/contrail-vrouter-agent.conf
      - module: agent_systemd_units
      - yc_pkg: opencontrail_common_packages

# NOTE(xelez): it seems that we don't need any of the packages below.
# They can be probably deleted after packages with loosened dependencies are installed
contrail_vrouter_agent_other_packages:
  yc_pkg.installed:
    - pkgs:
      - python-contrail-vrouter-api
      - python-opencontrail-vrouter-netns
      - contrail-vrouter-source
    - hold_pinned_pkgs: True
    - require:
      - yc_pkg: contrail_vrouter_agent_packages
