{% set unit = 'elliptics-storage-files' %}
{% set env =  grains['yandex-environment'] %}

include:
  - units.zcore

{% for file in pillar.get('elliptics-storage-files-755') %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
{% endfor %}

{% for file in pillar.get('elliptics-storage-files-644') %}
{{file}}:
  yafile.managed:
    - source: salt://files/elliptics-storage{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
{% endfor %}

{% for file in pillar.get('elliptics-storage-conf-files', []) %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
{% endfor %}

{% for file in pillar.get('elliptics-storage-exec-files', []) %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
{% endfor %}

{% for file in pillar.get('elliptics-storage-secrets-files', []) %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 600
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
{% endfor %}

{% for file in pillar.get('elliptics-storage-conf-dirs', []) %}
{{file}}:
  file.directory:
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
{% endfor %}

# {% if grains['fqdn'] in ['s389man.storage.yandex.net', 's371man.storage.yandex.net', 's398man.storage.yandex.net', 's387man.storage.yandex.net', 's375man.storage.yandex.net', 's364man.storage.yandex.net', 's379man.storage.yandex.net', 's367man.storage.yandex.net', 's391man.storage.yandex.net', 's402man.storage.yandex.net', 's373man.storage.yandex.net', 's400man.storage.yandex.net', 's382man.storage.yandex.net', 's395man.storage.yandex.net', 's388man.storage.yandex.net', 's392man.storage.yandex.net', 's359man.storage.yandex.net', 's351man.storage.yandex.net', 's353man.storage.yandex.net', 's355man.storage.yandex.net', 's404man.storage.yandex.net', 's352man.storage.yandex.net', 's381man.storage.yandex.net', 's384man.storage.yandex.net', 's374man.storage.yandex.net', 's358man.storage.yandex.net', 's401man.storage.yandex.net', 's394man.storage.yandex.net', 's396man.storage.yandex.net', 's383man.storage.yandex.net', 's360man.storage.yandex.net', 's368man.storage.yandex.net', 's399man.storage.yandex.net', 's393man.storage.yandex.net', 's378man.storage.yandex.net', 's397man.storage.yandex.net', 's362man.storage.yandex.net', 's403man.storage.yandex.net', 's369man.storage.yandex.net', 's376man.storage.yandex.net', 's380man.storage.yandex.net', 's405man.storage.yandex.net', 's390man.storage.yandex.net', 's377man.storage.yandex.net', 's386man.storage.yandex.net', 's385man.storage.yandex.net', 's372man.storage.yandex.net', 's357man.storage.yandex.net', 's354man.storage.yandex.net', 's370man.storage.yandex.net'] %}
# mds_7445:
#   file.managed:
#     - name: /etc/cron.d/mds_7445
#     - contents: >
#         */1 * * * * root flock -x -n /var/lock/mds_7445.lock -c '/usr/bin/sa-run-defrag-for-big-backend.sh -m 50 >/dev/null 2>/dev/null'
#     - user: root
#     - group: root
#     - mode: 755
# {%- endif %}

/etc/monitoring/elliptics-queue.conf:
  file.managed:
    - contents: |
        queue=4000
    - user: root
    - group: root
    - mode: 644

/etc/elliptics/sa/autoadmin.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/elliptics/sa/autoadmin.conf
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - template: jinja

/usr/bin/ns_link_usage.py:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/bin/ns_link_usage.py
    - mode: 755
    - user: root
    - group: root

ns-link-usage:
  monrun.present:
    - command: flock -w4600 /var/lock/backends_setup -c "/usr/bin/ns_link_usage.py -m -s 3600 -i mail,disk"
    - execution_interval: 7200
    - execution_timeout: 4900

/etc/elliptics/MDS-9412:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/elliptics/MDS-9412
    - user: root
    - group: root
    - mode: 644


bad_ids:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/local/bin/monrun_bad_ids.sh
    - name: /usr/local/bin/monrun_bad_ids.sh
    - mode: 755
    - user: root
    - group: root

  monrun.present:
    - command: /usr/local/bin/monrun_bad_ids.sh
    - execution_interval: 3600
    - execution_timeout: 1800

# MDS-18999 SAS
{% if grains['fqdn'] in ['s493sas.storage.yandex.net', 's494sas.storage.yandex.net', 's495sas.storage.yandex.net', 's496sas.storage.yandex.net', 's497sas.storage.yandex.net', 's498sas.storage.yandex.net', 's499sas.storage.yandex.net', 's501sas.storage.yandex.net', 's502sas.storage.yandex.net', 's504sas.storage.yandex.net', 's505sas.storage.yandex.net', 's508sas.storage.yandex.net', 's509sas.storage.yandex.net', 's512sas.storage.yandex.net', 's513sas.storage.yandex.net'] %}
mds_18999:
  file.managed:
    - name: /etc/cron.d/mds_18999
    - contents: |
        04 */2 * * * root flock -x -n /var/lock/mds_18999.lock -c '/usr/bin/move_groups.py --max_jobs 12 --random_sleep 3600 >/dev/null 2>/dev/null'
    - user: root
    - group: root
    - mode: 755
/var/tmp/sa-stop-create-backends:
  file.managed:
    - name: /var/tmp/sa-stop-create-backends
    - contents: >
        test
    - user: root
    - group: root
    - mode: 644
{%- endif %}

# MDS-18999 IVA
# {% if grains['fqdn'] in ['s276iva.storage.yandex.net', 's266iva.storage.yandex.net', 's273iva.storage.yandex.net', 's268iva.storage.yandex.net', 's263iva.storage.yandex.net', 's270iva.storage.yandex.net', 's267iva.storage.yandex.net', 's265iva.storage.yandex.net', 's274iva.storage.yandex.net', 's272iva.storage.yandex.net', 's264iva.storage.yandex.net', 's271iva.storage.yandex.net', 's275iva.storage.yandex.net', 's277iva.storage.yandex.net', 's269iva.storage.yandex.net'] %}
# mds_18999:
#   file.managed:
#     - name: /etc/cron.d/mds_18999
#     - contents: |
#         04 */2 * * * root flock -x -n /var/lock/mds_18999.lock -c '/usr/bin/move_groups.py --max_jobs 12 --random_sleep 3600 >/dev/null 2>/dev/null'
#     - user: root
#     - group: root
#     - mode: 755
# /var/tmp/sa-stop-create-backends:
#   file.managed:
#     - name: /var/tmp/sa-stop-create-backends
#     - contents: >
#         test
#     - user: root
#     - group: root
#     - mode: 644
# {%- endif %}

{% if grains['conductor']['root_datacenter'] in ['iva', 'myt'] and grains['yandex-environment'] != 'testing' %}
mds_13367:
  file.managed:
    - name: /etc/cron.d/mds_13367
    - contents: >
        04 */4 * * * root zk-flock mds_13367 -n 8 -w 120 '/usr/bin/move_groups.py --max_jobs 8 --move_lrc_groups_from_dc --random_sleep 24' >/dev/null 2>/dev/null
    - user: root
    - group: root
    - mode: 755
{%- endif %}

{% if grains['fqdn'] in ['s658sas.storage.yandex.net'] %}
mds_13408:
  file.managed:
    - name: /etc/cron.d/mds_13408
    - contents: >
        04 */2 * * * root flock -x -n /var/lock/mds_13408.lock -c '/usr/bin/move_groups.py --max_jobs 60 --random_sleep 3600 >/dev/null 2>/dev/null'
    - user: root
    - group: root
    - mode: 755
/var/tmp/sa-stop-create-backends:
  file.managed:
    - name: /var/tmp/sa-stop-create-backends
    - contents: >
        test
    - user: root
    - group: root
    - mode: 644
{%- endif %}

# {% if grains['conductor']['root_datacenter'] in ['man'] and grains['yandex-environment'] != 'testing' %}
# mds_18288:
#   file.managed:
#     - name: /etc/cron.d/mds_18288
#     - contents: >
#         */15 * * * * root zk-flock mds_18288 -n 24 -w 300 '/usr/bin/move_groups.py --max_jobs 10 --move_x2_groups_from_dc --preferable_dcs sas,vla,myt --random_sleep 4' >/dev/null 2>/dev/null
#     - user: root
#     - group: root
#     - mode: 755
# {%- endif %}

# MDS-19695
{% if grains['fqdn'] in ['s662man.storage.yandex.net', 's642man.storage.yandex.net', 's656man.storage.yandex.net', 's652man.storage.yandex.net', 's346man.storage.yandex.net', 's347man.storage.yandex.net', 's245man.storage.yandex.net', 's244man.storage.yandex.net', 's246man.storage.yandex.net', 's247man.storage.yandex.net', 's248man.storage.yandex.net', 's249man.storage.yandex.net', 's651man.storage.yandex.net', 's659man.storage.yandex.net', 's669man.storage.yandex.net', 's630man.storage.yandex.net', 's633man.storage.yandex.net', 's636man.storage.yandex.net', 's686man.storage.yandex.net', 's664man.storage.yandex.net', 's635man.storage.yandex.net', 's637man.storage.yandex.net', 's640man.storage.yandex.net', 's668man.storage.yandex.net', 's646man.storage.yandex.net', 's645man.storage.yandex.net', 's661man.storage.yandex.net', 's638man.storage.yandex.net', 's649man.storage.yandex.net', 's657man.storage.yandex.net', 's650man.storage.yandex.net', 's648man.storage.yandex.net', 's663man.storage.yandex.net', 's676man.storage.yandex.net', 's655man.storage.yandex.net', 's525man.storage.yandex.net', 's529man.storage.yandex.net', 's672man.storage.yandex.net', 's643man.storage.yandex.net', 's667man.storage.yandex.net', 's644man.storage.yandex.net', 's687man.storage.yandex.net', 's665man.storage.yandex.net', 's666man.storage.yandex.net', 's631man.storage.yandex.net', 's147man.storage.yandex.net', 's154man.storage.yandex.net', 's148man.storage.yandex.net', 's153man.storage.yandex.net', 's152man.storage.yandex.net', 's149man.storage.yandex.net', 's142man.storage.yandex.net', 's140man.storage.yandex.net', 's144man.storage.yandex.net', 's141man.storage.yandex.net', 's139man.storage.yandex.net', 's143man.storage.yandex.net', 's634man.storage.yandex.net', 's684man.storage.yandex.net', 's680man.storage.yandex.net', 's683man.storage.yandex.net', 's685man.storage.yandex.net', 's639man.storage.yandex.net', 's674man.storage.yandex.net', 's678man.storage.yandex.net', 's670man.storage.yandex.net', 's675man.storage.yandex.net', 's677man.storage.yandex.net', 's682man.storage.yandex.net', 's580man.storage.yandex.net', 's583man.storage.yandex.net', 's581man.storage.yandex.net', 's582man.storage.yandex.net', 's626man.storage.yandex.net', 's598man.storage.yandex.net', 's606man.storage.yandex.net', 's620man.storage.yandex.net', 's452man.storage.yandex.net', 's461man.storage.yandex.net', 's455man.storage.yandex.net', 's464man.storage.yandex.net', 's197i.storage.yandex.net', 's204i.storage.yandex.net', 's205i.storage.yandex.net', 's196i.storage.yandex.net', 's203i.storage.yandex.net', 's207i.storage.yandex.net', 's195i.storage.yandex.net', 's202i.storage.yandex.net', 's208i.storage.yandex.net', 's198i.storage.yandex.net', 's193i.storage.yandex.net', 's206i.storage.yandex.net', 's199i.storage.yandex.net', 's200i.storage.yandex.net', 's194i.storage.yandex.net', 's201i.storage.yandex.net', 's394man.storage.yandex.net', 's397man.storage.yandex.net', 's396man.storage.yandex.net', 's403man.storage.yandex.net', 's192i.storage.yandex.net', 's187i.storage.yandex.net', 's190i.storage.yandex.net', 's188i.storage.yandex.net', 's360man.storage.yandex.net', 's351man.storage.yandex.net', 's352man.storage.yandex.net', 's373man.storage.yandex.net', 's364man.storage.yandex.net', 's365man.storage.yandex.net', 's366man.storage.yandex.net', 's369man.storage.yandex.net', 's358man.storage.yandex.net', 's356man.storage.yandex.net', 's355man.storage.yandex.net', 's353man.storage.yandex.net', 's380man.storage.yandex.net', 's382man.storage.yandex.net', 's383man.storage.yandex.net', 's374man.storage.yandex.net'] %}
mds_18999:
  file.managed:
    - name: /etc/cron.d/mds_19695
    - contents: |
        04 */2 * * * root flock -x -n /var/lock/mds_18999.lock -c '/usr/bin/move_groups.py --new_queue_move_man --max_jobs 22 --random_sleep 3600 >/dev/null 2>/dev/null'
    - user: root
    - group: root
    - mode: 755
/var/tmp/sa-stop-create-backends:
  file.managed:
    - name: /var/tmp/sa-stop-create-backends
    - contents: >
        test
    - user: root
    - group: root
    - mode: 644
{%- endif %}
