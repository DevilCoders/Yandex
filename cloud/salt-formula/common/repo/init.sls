/etc/apt/sources.list:
  file.managed:
    - template: jinja
    - makedirs: True
    - source: salt://{{ slspath }}/files/sources.list

/etc/apt/sources.list.d/common-stable.list:
  file.absent:
    - name: /etc/apt/sources.list.d/common-stable.list

apt-update:
  cmd.run:
    - name: apt-get update
    - onchanges:
      - file: /etc/apt/sources.list
      - file: /etc/apt/sources.list.d/common-stable.list

/etc/apt/preferences.d/99-cloud-upstream-xenial-repo.pref:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath }}/files/99-cloud-upstream-xenial-repo.pref
    - onchanges_in:
      - cmd: apt-update

{% set environment = grains['cluster_map']['environment'] %}
{% for branches in pillar['cloudrepo_branches'][environment]['branches'] %}
{% set branch = branches.keys()[0] %}
{% set priority = branches.values()[0] %}
/etc/apt/preferences.d/99-cloud-{{ branch }}-repo.pref:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath }}/files/99-cloud-repo.pref
    - defaults:
        branch: {{ branch }}
        priority: {{ priority }}
    - onchanges_in:
      - cmd: apt-update

dist-yandex-cloud-{{ branch }}:
  pkgrepo.managed:
     - name: deb http://yandex-cloud.dist.yandex.ru/yandex-cloud {{ branch }}/all/
     - file: /etc/apt/sources.list.d/yandex-cloud-{{ branch }}.list

dist-yandex-cloud-{{ grains['osarch'] }}-{{ branch }}:
  pkgrepo.managed:
     - name: deb http://yandex-cloud.dist.yandex.ru/yandex-cloud {{ branch }}/{{ grains['osarch'] }}/
     - file: /etc/apt/sources.list.d/yandex-cloud-{{ grains['osarch'] }}-{{ branch }}.list
{% endfor %}

#TODO: remove after replacing CI template to vanilla Ubuntu template
/etc/apt/preferences:
  file.absent

/etc/apt/preferences.d/99-cloud-versions.pref:
  file.absent

/etc/apt/apt.conf.d/02ycloud_apt_config:
  file.managed:
    - template: jinja
    - makedirs: True
    - source: salt://{{ slspath }}/files/02ycloud_apt_config
