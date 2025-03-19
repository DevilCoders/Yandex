/etc/elliptics/mastermind-minion/mastermind-minion.conf:
  yafile.managed:
    - source: salt://{{ slspath }}/files/mastermind-minion.conf
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - template: jinja

/etc/logrotate.d/mastermind-minion:
  file.managed:
    - source: salt://{{ slspath }}/files/logrotate-mastermind-minion
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/etc/ubic/service/mastermind-minion.ini:
  file.managed:
    - source: salt://{{ slspath }}/files/ubic-mastermind-minion
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/etc/elliptics/mastermind-minion/logging.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/logging.conf
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/etc/syslog-ng/conf-enabled/30-mastermind-minion.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/syslog-ng-mastermind-minion.conf
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/usr/share/perl5/Ubic/Service/MastermindMinion.pm:
  file.managed:
    - source: salt://{{ slspath }}/files/MastermindMinion.pm
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

mastermind-minion:
  monrun.present:
    - command: "/usr/bin/jhttp.sh -u /ping/ -p 9000 -t 30"
    - execution_interval: 300
    - execution_timeout: 100
    - type: mastermind
