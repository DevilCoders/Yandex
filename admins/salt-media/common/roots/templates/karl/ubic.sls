/usr/share/perl5/Ubic/Service/Karl.pm:
  file.managed:
    - source: salt://{{ slspath }}/files/ubic/Karl.pm
    - mode: 644
    - makedirs: True

/etc/ubic/service/karl.ini:
  file.managed:
    - source: salt://{{ slspath }}/files/ubic/karl.ini
    - mode: 644
    - makedirs: True

/etc/init.d/karl:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/init.d/karl
    - mode: 755
    - makedirs: True

ubic-pkg:
  pkg.installed:
    - pkgs:
      - liblwp-protocol-http-socketunixalt-perl
    - skip_suggestions: True
    - install_recommends: False
