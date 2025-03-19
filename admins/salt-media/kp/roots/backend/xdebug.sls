{% set yaenv = grains['yandex-environment'] %}
{% set conductor_group = salt["grains.get"]("conductor:group") %}
{% if '-xdebug' in conductor_group or 'kp-dev-trusty' in conductor_group %}
{% set need_xdebug = True %}
{% else %}
{% set need_xdebug = False %}
{% endif %}

/etc/php/5.6/mods-available/xdebug.ini:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/php5/conf.d/xdebug.ini
    - makedirs: true
    - mode: 0755

xdebug_symlink:
{% if need_xdebug %}
  file.symlink:
    - target: /etc/php/5.6/mods-available/xdebug.ini
{% else %}
  file.absent:
{% endif %}
    - names:
      - /etc/php/5.6/cli/conf.d/20-xdebug.ini
      - /etc/php/5.6/fpm/conf.d/20-xdebug.ini
