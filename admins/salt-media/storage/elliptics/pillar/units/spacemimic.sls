{% set unit = 'spacemimic' %}

# ======= spacemimic ======= #

spacemimic-exec-files:
  - /etc/init.d/spacemimic

spacemimic-files:
  - /etc/elliptics/srw/mimic.conf
  - /etc/ubic/service/spacemimic.ini
  - /etc/logrotate.d/spacemimic
  - /usr/share/perl5/Ubic/Service/Spacemimic.pm

spacemimic-dirs:
  - /etc/elliptics/srw/
  - /var/log/elliptics/srw/spacemimic/

spacemimic-obsolete-files:
  - /etc/ubic/service/spacemimic
