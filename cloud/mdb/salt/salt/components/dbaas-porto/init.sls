/usr/local/yandex/pre_restart.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/pre_restart.sh
        - template: jinja
        - makedirs: True
        - mode: 700

/usr/local/yandex/post_restart.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/post_restart.sh
        - template: jinja
        - makedirs: True
        - mode: 700

include:
    - components.common.dns

{% if salt['grains.get']('virtual') == 'lxc' and salt['grains.get']('virtual_subtype', None) != 'Docker' %}
porto-resources-grains-pkgs:
    pkg.installed:
        - pkgs:
            - python-portopy: {{ salt.grains.get('porto_version', '4.18.21') }}
            - python3-portopy:  {{ salt.grains.get('porto_version', '4.18.21') }}
{% endif %}
