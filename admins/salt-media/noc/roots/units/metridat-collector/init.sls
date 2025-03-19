/etc/yandex/unified_agent/conf.d:
  file.recurse:
    - source: salt://{{ slspath }}/files/etc/yandex/unified_agent/conf.d
    - template: jinja
    - user: unified_agent
    - group: unified_agent
    - file_mode: 644
    - dir_mode: 755
    - makedirs: True
    - clean: True

/etc/yandex/unified_agent/secrets/solomon-oauth-token:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/yandex/unified_agent/secrets/solomon-oauth-token
    - template: jinja
    - user: unified_agent
    - group: unified_agent
    - mode: 600
    - makedirs: True

/etc/metridat-collector/config.yml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/metridat-collector/config.yaml
    - template: jinja
    - user: metridat
    - group: metridat
    - mode: 600
    - makedirs: True

/etc/metridat-collector/conf.d:
  file.recurse:
    - source: salt://{{ slspath }}/files/etc/metridat-collector/conf.d
    - template: jinja
    - user: metridat
    - group: metridat
    - file_mode: 600
    - dir_mode: 700
    - makedirs: True
    - clean: True
