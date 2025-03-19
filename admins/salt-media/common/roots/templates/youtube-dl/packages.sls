packages:
  pkg.installed:
    - pkgs:
      - 'python-pip'
      - 'python-setuptools'
      - 'yandex-conf-repo-media-common-stable'

updater:
  pkg.installed:
    - name: yandex-media-common-youtube-dl-updater
    - refresh: True
