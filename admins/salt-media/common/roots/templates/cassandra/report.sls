{% from  slspath + "/map.jinja" import cassandra with context %}

/etc/cassandra/report.yaml:
  file.managed:
    - contents: |
        graphite:
          -
            period: 60
            timeunit: 'SECONDS'
            prefix: media.{{ salt["grains.get"]("conductor:project") }}.{{ salt["grains.get"]("conductor:group") }}.{{ salt["grains.get"]("host") }}
            hosts:
             - host: 'localhost'
               port: 42000
            predicate:
              color: "white"
              useQualifiedName: true
              patterns:
                {{ cassandra.graphite.patterns|yaml(False)|indent(16) }}

{%- if cassandra.graphite.get("default_report_lib", True) %}
graphite_report:
  file.managed:
    - name: /usr/share/cassandra/lib/metrics-graphite-3.1.2.jar
    - source: salt://{{ slspath }}/usr/share/cassandra/lib/metrics-graphite-3.1.2.jar
{% endif %}
