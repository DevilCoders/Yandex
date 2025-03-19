kvm-pkgs:
    pkg.installed:
        - pkgs:
            - python3.6-dev
            - python3.6-venv
            - qemu-kvm
            - qemu-utils

/etc/cron.yandex/qemu_cleanup.sh:
    file.managed:
        - source: salt://{{ slspath + '/conf/qemu_cleanup.sh' }}
        - mode: 755

/etc/cron.d/qemu_cleanup:
    file.managed:
        - source: salt://{{ slspath + '/conf/qemu_cleanup.cron' }}

/etc/dbaas-vm-setup:
    file.directory:
        - mode: 700

/etc/dbaas-vm-setup/minion.pub:
    file.managed:
        - source: salt://{{ slspath + '/conf/minion.pub' }}
        - template: jinja
        - mode: 600

/etc/dbaas-vm-setup/minion.pem:
    file.managed:
        - source: salt://{{ slspath + '/conf/minion.pem' }}
        - template: jinja
        - mode: 600

/etc/dbaas-vm-setup/master_sign.pub:
    file.managed:
        - contents_pillar: data:kvm_host:template:master_pub
        - mode: 600
