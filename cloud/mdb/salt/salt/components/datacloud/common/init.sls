include:
    - .cores
    - .systemd
    - .monitor
    - .misc
    - .scripts
    - .repo
    - .pkgs_ubuntu
    - .mount
    - components.atop
    - components.repositories
    - components.mdb-telegraf
    - components.vector_agent_dataplane
    - components.firewall
    - components.disklock
    - components.datacloud.network
    - components.common.dbaas
{% if not salt['pillar.get']('data:salt_masterless', False) %}
    - components.deploy.mdb-ping-salt-master
{% endif %}
