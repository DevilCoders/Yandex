/etc/cron.d/compact_memory:
  yafile.managed:
    - source: salt://{{ slspath }}/files/compact_memory.cron
