include:
  - common.juggler-client

{% if grains['virtual'] != 'physical' %}
{# NOTE(k-zaitsev): A temporary state to disable netmon and it's monitoring on SVMs
   TODO: drop this state when deployed on all SVMs.
   See CLOUD-11467 for details when this will run on SVMs #}
/etc/monrun/conf.d/netmon-agent-health.conf:
  file.absent

netmon-agent-disable:
  service.dead:
    - enable: False
    - name: netmon-agent
{% endif %}

{% from slspath+"/monitoring_common.yaml" import monitoring %}
{% include "common/deploy_mon_scripts.sls" %}
{% if grains["virtual"] == "physical" %}
{% from slspath+"/monitoring_hw.yaml" import monitoring %}
{% include "common/deploy_mon_scripts.sls" %}
{% endif %}

{{ sls }}_packages:
  yc_pkg.installed:
    - pkgs:
      - runit
      - libfile-readbackwards-perl
      - python-requests
      - python3-dnspython
      - perl
      - perl-modules
      - libfile-slurp-perl
      - libjson-perl
      - libtry-tiny-perl
      - libwww-perl
      - libconfig-general-perl
      - dmidecode
      - python-dmidecode
      - jq
      - yandex-juggler-http-check
      - python-psutil
      - python3-defusedxml
      - monrun
      - juggler-client
    - require:
      - file: /etc/juggler/client.conf

/usr/local/lib/python3.5/dist-packages/yc_monitoring.py:
  file.managed:
    - source: salt://{{ slspath }}/mon/lib/yc_monitoring.py
    - mode: 644

{# FIXME(valesini): Move this to common.vpc.monitoring or alike; see CLOUD-23509 #}
/usr/local/lib/python3.5/dist-packages/yc_contrail_monitoring.py:
  file.managed:
    - source: salt://{{ slspath }}/mon/lib/yc_contrail_monitoring.py
    - mode: 644
