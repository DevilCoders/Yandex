run-data-move-script:
  cmd.run:
{% if salt['grains.get']('os_family') == 'Windows' %}
    - name: C:\Program Files\Mdb\data_move.ps1 back
{% else %}
    - name: /usr/local/yandex/data_move.sh back
{% endif %}
