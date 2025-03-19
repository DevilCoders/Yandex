{% set cgroup = grains['conductor']['group'] %}
{% set cgroups = grains['conductor']['groups'] %}

#ELLIPTICS:STORAGE
{% if 'elliptics-storage' in cgroup or 'elliptics-test-storage' in cgroup %}

include:
  - units.cocaine.config
  - units.cocaine.isolate
  - units.cocaine.pkgs
  - units.cocaine.fsstate
  - units.cocaine.osstate

#ELLIPTICS:PROXY
{% elif 'elliptics-test-proxies' in cgroups or 'elliptics-proxy' in cgroup %}

include:
  - units.cocaine.config
  - units.cocaine.pkgs
  - units.cocaine.fsstate
  - units.cocaine.osstate


#ELLIPTICS:MASTERMIND
{% elif 'elliptics-cloud' in cgroup or 'elliptics-test-cloud' in cgroup or 'collector' in cgroup %}

include:
  - units.cocaine.config
#  - units.cocaine.isolate
#  - units.cocaine.pkgs
#  - units.cocaine.fsstate
#  - units.cocaine.osstate


#APE:UNDERCONSTRUCTION
{% elif 'ape-' in cgroup %}

include:
  - units.cocaine.isolate

{% else %}

    test.fail_without_changes

{% endif %}

