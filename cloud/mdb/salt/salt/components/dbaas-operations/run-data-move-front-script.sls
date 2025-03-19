run-data-move-script:
  cmd.run:
{% if salt['grains.get']('os_family') == 'Windows' %}
    - name: C:\Program Files\Mdb\data_move.ps1 front
{% else %}
    - name: /usr/local/yandex/data_move.sh front
{% endif %}
