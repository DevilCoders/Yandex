ferm-ready:
    test.nop

{# After resetup all hosts to 18.04 we need to changes this state to service.* state #}
reload-ferm-rules:
    cmd.wait:
        - name: (/usr/local/bin/retry systemctl reload-or-restart ferm) || (rm -f /etc/ferm/ferm.conf && exit 1)
        - require:
            - test: ferm-ready
