include:
  - units.mondata
  - units.metridat-aggregator
  - units.juggler-checks.common
  - units.unified-agent

metridat-client:
  user.present:
    - name: metridat
  group.present:
    - name: metridat
  pkg.installed:
    - pkgs:
      - metridat-aggregator
