
srv-configured:
    test.nop

/srv:
{% if salt.pillar.get('data:salt_master:use_s3_images', True) %}
    cmd.run:
        - name: '/etc/cron.yandex/update_salt_image.py update --force'
        - require:
              - file: /etc/cron.yandex/update_salt_image.py
        - unless:
              - 'test -L /srv'
{% else %}
    file.directory:
        - mode: 777
        - user: root
        - group: root
{% endif %}
        - require_in:
              - test.srv-configured
