include:
    - components.monrun2.salt-master
    - components.monrun2.deploy.salt-master
    - components.logrotate
    - .srv
    - .master
{% if salt.pillar.get('data:salt_master:use_deploy', True) %}
    - .deploy
    - .salt-api
    - components.nginx
{% endif %}
