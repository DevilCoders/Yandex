user:
  user.present:
    - name: hw-watcher

/etc/hw_watcher/conf.d/token.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/token.conf
    - user: hw-watcher
    - group: hw-watcher
    - mode: 600
    - makedirs: True
    - template: jinja
