/etc/postfix/main.cf:
  file.managed:
    - mode: 644
    - template: jinja
    - makedirs: True
    - source: salt://common/postfix_main.cf

/etc/mailname:
  file.managed:
    - mode: 644
    - contents_grains: fqdn

/etc/aliases:
  file.managed:
    - source: salt://common/aliases.config
    - mode: 644
    - template: jinja
