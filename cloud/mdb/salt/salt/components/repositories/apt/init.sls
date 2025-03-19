/etc/apt/preferences.d/40dist-pin-priority:
    file.managed:
        - contents: |
            Package: *
            Pin: {{ salt.grains.filter_by({
                        'default': 'release o=mdb-bionic-secure',
                        'trusty': 'origin "dist.yandex.ru"',
                    }, 'oscodename') }}
            Pin-Priority: 1
        - mode: 644
        - user: root
        - group: root
        - order: 1
        - onchanges_in:
            - cmd: repositories-ready

/etc/apt/preferences.d/suppress-dist:
    file.absent:
        - order: 1
        - onchanges_in:
            - cmd: repositories-ready

/etc/apt/apt.conf.d/40no-recommends:
    file.managed:
        - source: salt://components/repositories/apt/conf/apt.conf.d/40no-recommends
        - template: jinja
        - mode: 644
        - user: root
        - group: root
        - order: 1
        - onchanges_in:
            - cmd: repositories-ready

/etc/apt/apt.conf.d/40allow-downgrades:
    file.managed:
        - contents:
            - APT::Get::allow-downgrades "true";
        - mode: 644
        - user: root
        - group: root
        - onchanges_in:
            - cmd: repositories-ready

/etc/apt/apt.conf.d/40force-overwrite:
    file.managed:
        - contents:
            - Dpkg::Options { "--force-overwrite"; }
        - mode: 644
        - user: root
        - group: root
        - onchanges_in:
            - cmd: repositories-ready
