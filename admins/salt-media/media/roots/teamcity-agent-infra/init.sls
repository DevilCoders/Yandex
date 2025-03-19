include:
  - templates.docker-ce
  - .repo
  - .packages
  - .network
  - .monrun
  - .ssh
  - .dupload
  - .scripts

docker:
  group.present:
    - addusers:
        - teamcity
  file.managed:
    - name: /home/teamcity/.docker/config.json
    - user: teamcity
    - group: dpt_virtual_robots
    - mode: 0600
    - makedirs: True
    - contents: {{ salt['pillar.get']('yav:docker_config') | json }}

teamcity-agent:
  pkg.installed

install_teamcity_agent:
  cmd.run:
    - name: /etc/init.d/teamcity-agent install "{{ grains['fqdn'] }}" https://teamcity.yandex-team.ru
    - unless: test -d /usr/local/teamcity-agents/{{ grains['fqdn'] }}
    - require:
      - pkg: teamcity-agent

run_teamcity_agent:
  cmd.run:
    - name: /etc/init.d/teamcity-agent start "{{ grains['fqdn'] }}"
    - unless: {{ grains["ps"] }} | grep -v grep | grep -q "java.*{{ grains['fqdn'] }}"
    - require:
      - pkg: teamcity-agent
      - cmd: install_teamcity_agent

tweak_startup_script:
  cmd.run:
    - name: sed -i "s/teamcity_agent_name=\"buildAgent\"/teamcity_agent_name=\"$(hostname -f)\"/g" /etc/init.d/teamcity-agent
    - unless: grep -q "teamcity_agent_name=\"$(hostname -f)\"" /etc/init.d/teamcity-agent
    - require:
      - pkg: teamcity-agent
      - cmd: install_teamcity_agent
      - cmd: run_teamcity_agent
