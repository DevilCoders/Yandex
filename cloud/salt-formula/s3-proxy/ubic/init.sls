ubic:
  yc_pkg.installed:
    - pkgs:
      - ubic
      - libyandex-ubic-shared-perl
      - libnet-inet6glue-perl
      - libwww-perl
      - liblwp-protocol-https-perl
      - liblwp-protocol-http-socketunixalt-perl

{%- for config_name in ['s3-lf-scheduler', 's3-lf-worker', 's3-mds-proxy-arc', 's3-mds-yc-idm-arc'] %}
/etc/ubic/service/{{ config_name }}.json:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/ubic_configs/{{ config_name }}
    - template: jinja
{%- endfor %}

{%- for service_name in ['S3MdsProxyArc.pm', 'S3MdsYcIDMArc.pm', 'S3MdsLFSched.pm', 'S3MdsLFWorker.pm'] %}
/usr/share/perl5/Ubic/Service/{{ service_name }}:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/ubic_services/{{ service_name }}
    - template: jinja
{%- endfor %}

{%- for ubic_init in ['s3-mds-proxy-arc', 's3-mds-yc-idm-arc', 's3-lf-scheduler', 's3-lf-worker'] %}
/etc/init.d/{{ ubic_init }}:
  file.managed:
    - makedirs: True
    - mode: 755
    - source: salt://{{ slspath }}/ubic_services/init
{%- endfor %}
