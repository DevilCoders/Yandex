/etc/cron.d/var-mail-cleanup:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/cron.d/var-mail-cleanup
        - mode: 644
        - user: root
        - group: root

{% if not (salt['grains.get']('virtual', 'physical') == 'lxc' and salt['grains.get']('virtual_subtype', None) == 'Docker') %}
/etc/aliases:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/aliases
        - mode: 644
        - user: root
        - group: root

newaliases:
    cmd.wait:
        - watch:
            - file: /etc/aliases

/etc/virtual_aliases:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/virtual_aliases
        - template: jinja
        - mode: 644
        - user: root
        - group: root

'postmap /etc/virtual_aliases':
    cmd.wait:
        - watch:
            - file: /etc/virtual_aliases

/etc/postfix/main.cf:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/postfix/main.cf
        - template: jinja
        - mode: 644
        - user: root
        - group: root

postfix-service:
    service.running:
        - name: postfix
        - enable: true
        - watch:
            - file: /etc/virtual_aliases
            - file: /etc/aliases
            - file: /etc/postfix/main.cf
        - require:
            - cmd: newaliases
            - cmd: 'postmap /etc/virtual_aliases'
            - pkg: common-packages
{% endif %}
