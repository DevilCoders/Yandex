/usr/share/perl5/Ubic/Service/YARL.pm:
  file.managed:
    - source: salt://{{ slspath }}/files/ubic/YARL.pm
    - mode: 644
    - makedirs: True

/etc/ubic/service/yarl.json:
  file.managed:
    - source: salt://{{ slspath }}/files/ubic/yarl.json
    - mode: 644
    - makedirs: True

/etc/init.d/yarl:
  file.managed:
    - source: salt://{{ slspath }}/files/ubic/yarl
    - mode: 755
    - makedirs: True

stop-yarl:
  cmd.run:
    - name: /usr/bin/killall -TERM yarl
    - onchanges:
      - file: /etc/init.d/yarl

