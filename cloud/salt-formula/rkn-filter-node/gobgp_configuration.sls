{% set gobgp_systemd_file_path = "jinja_templates/rootfs/etc/systemd/system/gobgp.service.j2" %}
{% set gobgp_configuration_file_path = "jinja_templates/rootfs/etc/gobgp/gobgpd.conf.j2" %}
{% include "netinfra/generic_netinfra_gobgp_configuration.sls" %}