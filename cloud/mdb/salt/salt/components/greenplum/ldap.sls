/etc/ldap/ldap.conf:
  file.managed:
    - source: salt://{{ slspath }}/conf/ldap.conf
    - template: jinja
    - mode: 644
