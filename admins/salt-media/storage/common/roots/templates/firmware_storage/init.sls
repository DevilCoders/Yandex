{% set unit = 'firmware_storage' %}

{% for file in ['ubuntu1110.deb', 'ubuntu1204.deb', 'ubuntu1304.deb', 'ubuntu1404.deb'] %}
/usr/local/lsi/{{file}}:
  yafile.managed:
    - source: salt://templates/firmware_storage/{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
{% endfor %}

{% for file in ['9205.bin', '9205.rom', '9207.bin', '9207.rom', 'fwa10', 'fwa9', 'mfghub', 'mfgleft', 'mfgright', 'readme.pdf', '9200-8e.bin', '9200.rom'] %}
/usr/local/lsi/{{file}}:
  yafile.managed:
    - source: salt://templates/firmware_storage/{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
{% endfor %}

{% for exec_file in ['9205.sh', '9207.sh', 'sas2flash', 'xflash', '9200.sh', 'all.sh'] %}
/usr/local/lsi/{{exec_file}}:
  yafile.managed:
    - source: salt://templates/firmware_storage/{{ exec_file }}
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
{% endfor %}
