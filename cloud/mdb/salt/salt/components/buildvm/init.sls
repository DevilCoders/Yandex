{% set image_path = '/var/cache/pbuilder' %}
{% set user = 'robot-pgaas-ci' %}

pyenv-dependecies:
    pkg.latest:
        - refresh: False
        - pkgs:
            - libreadline-dev
            - libbz2-dev
            - libssl-dev
            - libsqlite3-dev
        - prereq_in:
            - cmd: repositories-ready

buildvm-packages:
    pkg.latest:
        - pkgs:
            - pbuilder
            - debootstrap
            - devscripts
            - dh-make
            - dupload
            - dpkg
            - mingw-w64
        - refresh: False
        - prereq_in:
            - cmd: repositories-ready

packer-package:
    pkg.installed:
        - pkgs:
            - packer: 1.4.3+yandex0

gcc-9-packages:
    pkg.latest:
        - pkgs:
            - gcc-9
            - g++-9
        - refresh: False
        - prereq_in:
            - cmd: repositories-ready

set-gcc-to-9:
    cmd.run:
        - name: update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 10
        - unless:
            - ls -la /etc/alternatives/gcc 2>/dev/null | grep -q gcc-9
        - require:
            - pkg: gcc-9-packages

set-g++-to-9:
    cmd.run:
        - name: update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 10
        - unless:
            - ls -la /etc/alternatives/g++ 2>/dev/null | grep -q g++-9
        - require:
            - pkg: gcc-9-packages

/usr/lib/pbuilder/hooks/E00set-gcc9:
    file.managed:
        - source: salt://{{ slspath }}/conf/pbuilder-gcc-hook
        - makedirs: True
        - mode: '0755'

{# RC file for local shipping via `salt-call state.highstate ...` #}
/root/.pbuilderrc:
    file.managed:
        - source: salt://{{ slspath }}/conf/pbuilderrc
        - template: jinja
        - defaults:
            image_path: {{ image_path }}
        - require:
            - pkg: buildvm-packages

{# RC file for shipping from salt-web #}
/.pbuilderrc:
    file.managed:
        - source: salt://{{ slspath }}/conf/pbuilderrc
        - template: jinja
        - defaults:
            image_path: {{ image_path }}
        - require:
            - pkg: buildvm-packages

{# RC file for jenkins agent user #}
/home/{{ user }}/.pbuilderrc:
    file.managed:
        - user: {{ user }}
        - source: salt://{{ slspath }}/conf/pbuilderrc
        - template: jinja
        - defaults:
            image_path: {{ image_path }}
        - require:
            - pkg: buildvm-packages

{% for distrib, repo in salt['pillar.get']('data:buildvm:dist_map', {}).items() %}
{% set image = '{path}/{distro}-amd64-base.tgz'.format(distro=distrib, path=image_path) %}
{%
   set mirrors = [
       'deb http://mirror.yandex.ru/ubuntu {distr}-updates main restricted universe multiverse'.format(distr=distrib),
       'deb http://mirror.yandex.ru/ubuntu {distr}-security main restricted universe multiverse'.format(distr=distrib),
       'deb http://dist.yandex.ru/{repo} stable/all/'.format(repo=repo),
       'deb http://dist.yandex.ru/{repo} stable/\$(ARCH)/'.format(repo=repo),
   ]
%}

{% set othermirror = '|'.join(mirrors) %}
{{ distrib }}-image:
    cmd.run:
        - name: pbuilder --create --distribution {{ distrib }} --architecture amd64 --basetgz {{ image }} --override-config --othermirror "{{ othermirror }}" --debootstrapopts --include=gnupg
        - unless: test -s {{ image }}
        - require:
            - pkg: buildvm-packages
            - file: /.pbuilderrc
            - file: /root/.pbuilderrc
            - file: /home/{{ user }}/.pbuilderrc
            - file: /usr/lib/pbuilder/hooks/E00set-gcc9

/etc/cron.d/pbuilder_update_{{ distrib }}:
    file.managed:
        - contents: |
             PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
             10	*	*	*	*	root	sleep $((RANDOM \% 300)) && ARCH=amd64 DIST={{ distrib }} flock -w 60 /tmp/pbuilder-amd64-{{ distrib }}.lock timeout 1800 pbuilder update >/dev/null 2>&1
        - mode: 644
{% endfor %}

/var/cache/pbuilder/trusty-amd64-base.tgz:
    file.absent

/home/{{ user }}/.gnupg:
    file.directory:
        - user: {{ user }}
        - makedirs: True
        - mode: 700

/home/{{ user }}/.gnupg/key.armor:
    file.managed:
        - contents_pillar: 'data:buildvm:gpg_key'
        - user: {{ user }}
        - mode: 600
        - require:
            - file: /home/{{ user }}/.gnupg

drop-existing-keyrings:
    cmd.wait:
        - name: rm -rf /home/{{ user }}/.gnupg/*.gpg*
        - watch:
            - file: /home/{{ user }}/.gnupg/key.armor

import-build-gpg-key:
    cmd.wait:
        - runas: {{ user }}
        - name: gpg --no-tty --import /home/{{ user }}/.gnupg/key.armor
        - watch:
            - cmd: drop-existing-keyrings

/usr/local/yandex/make_deb.sh:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/make_deb.sh
        - mode: 755
        - user: {{ user }}
        - makedirs: True
        - require:
            - file: /home/{{ user }}/.pbuilderrc
            - cmd: import-build-gpg-key

/usr/local/yandex/resign_deb.sh:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/resign_deb.sh
        - mode: 755
        - user: {{ user }}
        - makedirs: True
        - require:
            - file: /home/{{ user }}/.pbuilderrc
            - cmd: import-build-gpg-key

/usr/local/yandex/make_deb_go.sh:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/make_deb_go.sh
        - mode: 755
        - user: {{ user }}
        - makedirs: True
        - require:
            - file: /home/{{ user }}/.pbuilderrc
            - cmd: import-build-gpg-key

/usr/bin/pdebuild_go:
    file.managed:
        - source: salt://{{ slspath }}/conf/pdebuild_go
        - mode: 755
        - user: root
        - require:
            - file: /home/{{ user }}/.pbuilderrc
            - cmd: import-build-gpg-key

include:
    - components.arcadia-build-node
    - components.repositories.apt.ubuntu-toolchain-r
