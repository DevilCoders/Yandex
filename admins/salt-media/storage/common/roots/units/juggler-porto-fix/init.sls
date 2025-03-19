/etc/default/juggler-client:
  file.managed:
    - source: salt://units/juggler-porto-fix/juggler-client
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
