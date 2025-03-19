run-post-restart-script:
  cmd.run:
{% if salt['grains.get']('os_family') == 'Windows' %}
    - name: 'powershell -file "C:\Program Files\Mdb\post_restart.ps1"'
{% else %}
    - name: /usr/local/yandex/post_restart.sh
{% endif %}
    - env:
        - TIMEOUT: {{ salt['pillar.get']('post-restart-timeout', 600) }}
