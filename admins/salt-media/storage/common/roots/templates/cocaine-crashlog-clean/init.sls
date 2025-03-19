{% set unit = 'cocaine-crashlog-clean' %}

/etc/cron.d/cocaine-crashlog-clean:
  file.managed:
    - source: salt://{{ slspath }}/files/cocaine-crashlog-clean
    - template: jinja
