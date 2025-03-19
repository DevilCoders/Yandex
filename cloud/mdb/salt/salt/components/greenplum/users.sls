{% from "components/greenplum/map.jinja" import gpdbvars with context %}

{{ gpdbvars.gpadmin }}:
  user.present:
    - home: /home/{{ gpdbvars.gpadmin }}
    - createhome: True
    - shell: /bin/bash
    - system: True

cores:
    group.present:
        - members:
            - {{ gpdbvars.gpadmin }}

{% if salt.pillar.get('restore-from:cid') %}
/home/{{ gpdbvars.gpadmin }}/.pgpass_restore:
  file.managed:
    - source: salt://{{ slspath }}/conf/pgpass_restore
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0600
    - template: jinja
    - require:
      - user: {{ gpdbvars.gpadmin }}
{% else %}
/home/{{ gpdbvars.gpadmin }}/.pgpass_restore:
  file.absent
{% endif %}

/home/{{ gpdbvars.gpadmin }}/.pgpass:
  file.managed:
    - source: salt://{{ slspath }}/conf/pgpass
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0600
    - template: jinja
    - require:
      - user: {{ gpdbvars.gpadmin }}

/root/.pgpass:
  file.managed:
    - source: salt://{{ slspath }}/conf/pgpass
    - user: root
    - group: root
    - mode: 0600
    - template: jinja

/home/{{ gpdbvars.gpadmin }}/.ssh:
  file.directory:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0750
    - require:
      - user: {{ gpdbvars.gpadmin }}

{% if not salt['pillar.get']('data:running_on_template_machine', False) %}
/home/{{ gpdbvars.gpadmin }}/.ssh/id_rsa:
  file.managed:
    - template: jinja
    - source: salt://components/greenplum/conf/ssh/id_rsa
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0400
    - makedirs: True
    - require:
      - user: {{ gpdbvars.gpadmin }}
      - file: /home/{{ gpdbvars.gpadmin }}/.ssh

? {{ gpdbvars.gpadmin_pub_key }}

:
  ssh_auth.present:
    - user: {{ gpdbvars.gpadmin }}
    - require:
      - user: {{ gpdbvars.gpadmin }}
{% endif %}

/home/{{ gpdbvars.gpadmin }}/.ssh/config:
  file.managed:
    - contents:
      - StrictHostKeyChecking no
      - GSSAPIAuthentication no
      - GSSAPIKeyExchange no
    - create: True
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0640
    - makedirs: True

/etc/security/limits.d/{{ gpdbvars.gpadmin }}.conf:
  file.managed:
    - source: salt://{{ slspath }}/conf/pam_limits
    - create: True
    - user: root
    - group: root
    - mode: 0644
    - template: jinja

