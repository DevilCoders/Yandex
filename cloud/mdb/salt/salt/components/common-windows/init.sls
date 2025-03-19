include:
{% if not salt['pillar.get']('data:running_on_template_machine', False) %}
{% if salt.pillar.get('data:os_clustered',true) == true %}
    - .cluster
{% endif %}
{% endif %}
    - .firewall
    - .ntp
    - .repositories
    - .packages
    - .internal-ca
    - .environment
    - .deploy
    - .ssh
    - .scheduler
    - .dbaas
    - .ps
    - .lgpo
