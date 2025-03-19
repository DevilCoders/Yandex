{% set prefix = "/usr/local" if grains["kernel"] == "FreeBSD" else "" %}

{{prefix}}/etc/rsyslog.d/00-global.conf:
  file.managed:
    - mode: 644
    - dir_mode: 750
    - makedirs: True
    - contents: |
        global(
          maxMessageSize="64K"
        )
