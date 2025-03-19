cocainerepo:
  pkg.installed:
    - pkgs:
      - yandex-conf-repo-cocainev12-trusty-testing
    - skip_suggestions: True
    - install_recommends: False

cocaine:
  user.present:
    - shell:
    - home: /home/cocaine
    - system: True
    - gid: adm
  pkg.installed:
    - pkgs:
      - cocaine-v12-framework-python
    - skip_suggestions: True
    - install_recommends: False

ubic:
  pkg.installed:
    - pkgs:
      - ubic
    - skip_suggestions: True
    - install_recommends: False

/etc/ubic/service/cocaine-orchestrator-server:
  file.managed:
    - source: salt://orchestrator-server/etc/ubic/service/cocaine-orchestrator-server
    - template: jinja

/usr/lib/python2.7/dist-packages/cocaine/orchestrator:
  file.recurse:
    - source: salt://common-files/cocaine-orchestrator-v.deep.alpha

/var/log/ubic/cocaine-orchestrator-server:
  file.directory:
    - user: cocaine

/etc/logrotate.d/cocaine-orchestrator-server:
  file.managed:
    - source: salt://orchestrator-server/etc/logrotate.d/cocaine-orchestrator-server
