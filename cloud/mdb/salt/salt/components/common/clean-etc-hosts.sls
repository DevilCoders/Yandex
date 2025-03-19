{# Warning this state is not idempotent #}

remove-fqdn-from-etc-hosts:
    file.line:
        - name: /etc/hosts
        - mode: delete
        - content: yandex.net

reload-bind9:
    cmd.run:
        - name: service bind9 reload || service bind9 restart

refresh-grains-after-bind9-reload:
    module.run:
        - name: saltutil.refresh_grains
        - refresh_pillar: false
        - require:
            - cmd: reload-bind9
            - file: remove-fqdn-from-etc-hosts
        - require_in:
            - host: fqdn-in-etc-hosts
