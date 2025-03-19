{%- from slspath + "/map.jinja" import docker with context -%}

/etc/apt/sources.list.d/docker.list:
  file.managed:
    - order: 0
    - contents: deb [arch=amd64] https://mirror.yandex.ru/mirrors/docker {{ grains['oscodename'] }} stable
/etc/apt/trusted.gpg.d/download.docker.com.gpg:
  file.decode:
    - order: 0
    - encoding_type: base64
    - encoded_data: |
        {{ docker.gpg_repo_key | indent(8) }}

docker-stock:
  {%- if docker.version %}
  pkg.installed:
    - name: docker-ce
    - version: {{ docker.version }}
    - refresh: True
  {%- else %}
    {% if docker.upgrade %}
  pkg.latest:
    - refresh: True
    {% else %}
  pkg.installed:
    {% endif %}
    - name: docker-ce
  {% endif %}
  {% if docker.legacy %}
  file.managed:
    - name: /etc/default/docker
    - source: salt://{{ slspath }}/docker
  {% else %}
  file.serialize:
    - name: /etc/docker/daemon.json
    - dataset: {{ docker.daemon_opts }}
    - formatter: json
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
  {% endif %}
  service.running:
    - name: docker
    - enable: True
    - require:
      - pkg: docker-stock
      - file: docker-stock
    - watch:
      - file: docker-stock
