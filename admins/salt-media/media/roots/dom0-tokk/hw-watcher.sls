/etc/hw_watcher/conf.d/tokk.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - contents: |
        [bot]
        oauth_token = {{ salt['pillar.get']('hw_watcher:token') }}
        initiator = {{ salt['pillar.get']('hw_watcher:user') }}
