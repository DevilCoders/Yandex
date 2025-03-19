include:
  - .systemd-reload

{% if salt.pillar.get('data:database_slice:enable', False) and salt.dbaas.is_dataplane() %}
/etc/systemd/system/database.slice:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/database.slice
        - onchanges_in:
            - module: systemd-reload
{% else %}
/etc/systemd/system/database.slice:
    file.absent:
        - onchanges_in:
            - module: systemd-reload
{% endif %}

{% if salt.grains.get('virtual', '') == 'lxc' and salt.grains.get('virtual_subtype', None) != 'Docker' %}
/etc/database-slice-adjuster.conf:
{%     if salt.pillar.get('data:database_slice:enable', False) and salt.dbaas.is_dataplane() %}
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/database-slice-adjuster.conf
{%     else %}
    file.absent
{%     endif %}
{% endif %}
