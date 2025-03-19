
mdb-deploy-agent-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-deploy-agent: '1.9621861'
        - require:
            - cmd: repositories-ready

/etc/yandex/mdb-deploy/agent.yaml:
    file.managed:
        - source: salt://{{ slspath }}/conf/agent.yaml
        - template: jinja
        - mode: 640


# Set order=last and no_block=True, because we need to restart the agent at the last moment.
# Cause agent restart will kill the running salt-call.
# Agent will wait for the commands to finish, but it waits no more than an hour.
mdb-deploy-agent-service:
    service.running:
        - name: mdb-deploy-agent
        - enable: True
        - no_block: True
        - watch:
            - pkg: mdb-deploy-agent-pkgs
            - file: /etc/yandex/mdb-deploy/agent.yaml
        - order: last

{% if salt.pillar.get('data:use_monrun', True) %}
include:
    - components.monrun2.deploy.agent
{% endif %}
