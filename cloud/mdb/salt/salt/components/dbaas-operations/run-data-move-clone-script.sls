run-data-move-script:
  cmd.run:
{% if salt['grains.get']('os_family') == 'Windows' %}
    - name: C:\Program Files\Mdb\data_move.ps1 clone
{% else %}
    - name: /usr/local/yandex/data_move.sh clone
{% endif %}
