{% set iam_conf_dir = "/etc/iam-token-reissuer" %}
{% set credentials_path = salt['pillar.get']('data:iam-token-reissuer:credentials-path', 'data:solomon_cloud') %}

iam-token-reissuer:
  pkg.installed:
    - version: '1.7625277'
    - prereq_in:
      - cmd: repositories-ready

{{ iam_conf_dir }}:
  file.directory:
    - user: root
    - group: root
    - mode: 0755

{{ iam_conf_dir }}/iam-token-reissuer.yaml:
  file.managed:
    - source: salt://{{ slspath }}/conf/iam_token_reissuer.yaml
    - user: root
    - group: root
    - mode: 0644
    - template: jinja
    - require:
      - file: {{ iam_conf_dir }}
    - context:
      iam_conf_dir: {{ iam_conf_dir }}
      credentials_path: {{ credentials_path }}

{{ iam_conf_dir }}/sa-private-key.key:
  file.managed:
    - contents_pillar: {{ credentials_path }}:sa_private_key
    - user: root
    - group: root
    - mode: 0640
    - require:
      - file: {{ iam_conf_dir }}

{{ iam_conf_dir }}/iam-token.txt:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - require:
      - file: {{ iam_conf_dir }}

iam-scheduled-task-extra-run:
  cmd.run:
    - name: /usr/bin/iam_token_reissuer --config {{ iam_conf_dir }}/iam-token-reissuer.yaml
    - onchanges:
      - file: {{ iam_conf_dir }}/iam-token-reissuer.yaml
    - require:
      - pkg: iam-token-reissuer

iam-reissuer-scheduled:
  cron.present:
    - name: /usr/bin/iam_token_reissuer --config {{ iam_conf_dir }}/iam-token-reissuer.yaml 
    - user: root
    - minute: 0
    - hour: '*/4'
    - require:
      - file: {{ iam_conf_dir }}/iam-token-reissuer.yaml
      - pkg: iam-token-reissuer

