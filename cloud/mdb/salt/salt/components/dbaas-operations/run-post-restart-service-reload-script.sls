run-post-restart-script:
  cmd.run:
{% if salt['grains.get']('os_family') == 'Windows' %}
    - name: C:\Program Files\Mdb\post_restart.ps1 service_reload
{% else %}
    - name: /usr/local/yandex/post_restart.sh service_reload
{% endif %}
    - env:
        - TIMEOUT: {{ salt['pillar.get']('post-restart-timeout', 600) }}
