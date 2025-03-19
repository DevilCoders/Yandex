{% macro ubic_service(name, bin, auto_start=1) %}
/var/log/ubic/{{name}}:
  file.directory:
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

/etc/logrotate.d/ubic-{{name}}:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - contents: |
        /var/log/ubic/{{name}}/*.log
        {
                weekly
                maxsize 100M
                compress
                dateext
                dateformat -%Y%m%d-%s
                missingok
                copytruncate
                rotate 15
        }

/etc/ubic/service/{{name}}:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - require:
      - pkg: ubic
    - watch_in:
      - service: {{name}}
    - template: jinja
    - contents: |  
        use Ubic::Service::SimpleDaemon;
        my $service = Ubic::Service::SimpleDaemon->new(
            name => '{{name}}',
            auto_start => {{auto_start}},
            bin => '{{bin}}',
            ubic_log => '/var/log/ubic/{{name}}/ubic.log',
            stdout => '/var/log/ubic/{{name}}/stdout.log',
            stderr => '/var/log/ubic/{{name}}/stderr.log',
        );

/etc/init.d/{{name}}:
  file.managed:
    - user: root
    - group: root
    - mode: 755
    - require:
      - file: /etc/ubic/service/{{name}}
    - contents: |
        #!/usr/bin/perl
        use Ubic::Run;

ubic-service-{{name}}:
  service.running:
    - name: {{name}}
    - enable: True
    - reload: False
    - require:
      - file: /etc/init.d/{{name}}

{% endmacro %}
