run-pre-restart-script:
  cmd.run:
{% if salt['grains.get']('os_family') == 'Windows' %}
    - name: 'powershell -file "C:\Program Files\Mdb\pre_restart.ps1"'
{% else %}
    - name: /usr/local/yandex/pre_restart.sh
{% endif %}
