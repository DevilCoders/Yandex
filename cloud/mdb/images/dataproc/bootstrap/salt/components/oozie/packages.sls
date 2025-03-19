oozie_packages:
  pkg.installed:
    - refresh: False
    - pkgs:
      - oozie
      - oozie-client

{% set path = '/usr/lib/oozie' %}
{% set mirror = salt['pillar.get']('data:repository:mirror', 'storage.yandexcloud.net') %}

{{ path }}:
    file.directory:
        - makedirs: True

# Oozie requires ExtJS 2.2 library with GPL
# Details https://issues.apache.org/jira/browse/OOZIE-2230
{{ path }}/ext.zip:
    file.managed:
        - source:
# Download archive from repository on presetup and use cached archive from image on typical setup
{% if salt['ydputils.is_presetup']() %}
            - https://{{ mirror }}/dataproc/gplextras/misc/ext-2.2.zip
{% else %}
            - {{ path }}/ext.zip

{% endif %}
        - source_hash: c463e298c7cc6fe3c5c863424a688b53a452bfe8f136bf20002e15b2bd201369
        - require:
            - file: {{ path }}
