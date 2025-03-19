jaeger-agent:
  pkg.installed:
    - version: {{ salt['pillar.get']('packages:jaeger-agent') }}
  service.running:
  - name: jaeger-agent
  - enable: true
  - require:
    - pkg: jaeger-agent
  - watch:
    - file: /etc/jaeger/agent.yaml

/etc/jaeger/agent.yaml:
  file.managed:
  - template: jinja
  - makedirs: true
  - source: salt://{{ slspath }}/files/config.yaml
  - require:
    - pkg: jaeger-agent
