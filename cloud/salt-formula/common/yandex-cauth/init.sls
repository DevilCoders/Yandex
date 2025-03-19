yandex-cauth:
  yc_pkg.installed:
    - pkgs:
      - yandex-cauth

{% if grains['virtual'] == "physical" %}
/etc/cauth/cauth.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/cauth.conf
    - require:
      - yc_pkg: yandex-cauth
{% endif %}
