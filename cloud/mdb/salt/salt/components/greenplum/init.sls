include:
    - .pkgs
    - .users
    - components.logrotate
{% if salt['pillar.get']('service-restart') %}
    - .restart
{% endif %}
{% if salt['pillar.get']('data:dbaas:cluster') %}
    - .resize
{% endif %}
{% if not salt['pillar.get']('data:running_on_template_machine', False) and salt['pillar.get']('data:use_telegraf', True) %}
    - components.mdb-telegraf
    - .telegraf
{%   if salt['grains.get']('virtual', 'physical') != 'lxc' and not salt['pillar.get']('data:lxc_used', False)  and salt['grains.get']('virtual_subtype', None) != 'Docker' %}
    - components.iam-token-reissuer
    - .mount_data_disks
    - .cgroups
    - .sysctl
    - .sys
    - .network
{%     if salt['pillar.get']('data:dbaas:vtype') == 'compute' %}
    - components.firewall
    - .firewall
{%     endif %}
{%   endif %}
{% endif %}
    - .install_greenplum
{% if salt.pillar.get('data:dbaas:subcluster_name') == 'master_subcluster' %}
    - .gp_autorecovery
{% endif %}
    - .service
    - .configure_greenplum
{% if not salt['pillar.get']('data:running_on_template_machine', False) %}
{%   if salt['pillar.get']('data:gpdb_ssl', True) %}
    - .ssl
{%   endif %}
    - .init_greenplum
    - .add_standby_greenplum
{% endif %}
    - .install_gpbackup
    - .install_pxf
{% if not salt['pillar.get']('data:running_on_template_machine', False) %}
    - .init_pxf
    - .configs
    - .create_user_gpdb
    - .extensions
    - .sqls
    - .ldap
    - .maintenance
{%   if salt.pillar.get('data:dbaas:subcluster_name') == 'master_subcluster' %}
    - components.odyssey.odyssey-gp
    - .odyssey
{%   endif %}
    - .ready
    - .walg
{% endif %}
{% if salt.pillar.get('gpdb_master', False) %}
    - .reload_greenplum_cluster
{% endif %}
{% if salt['pillar.get']('data:diagnostic_tools', True) %}
    - .diagnostics
{% endif %}
{% if salt.pillar.get('data:mvideo', False) %}
{%   if salt.pillar.get('data:dbaas:subcluster_name') == 'master_subcluster' %}
# Mvideo only related stuff
    - .mvideo
{%   endif %}
{% endif %}
    - .pushclient
    - components.pushclient2
    - .billing
