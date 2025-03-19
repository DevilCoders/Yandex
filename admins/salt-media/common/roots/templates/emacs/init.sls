{% set version = '24' %}
{% if grains['oscodename'] == 'bionic' %}
    {% set version = '25' %}
{% endif %}
emacs{{ version }}-nox:
  pkg.installed
yaml-mode:
  pkg.installed
php-elisp:
  pkg.installed

/root/.emacs:
  file.managed:
    - source: salt://{{slspath}}/files/emacs
    - mode: 0644
    - user: root
    - group: root
    - replace: True
