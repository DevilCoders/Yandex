yc-api-gateway:
  yc_pkg.installed:
    - pkgs:
      - yc-api-gateway

include:
  - .als
  - .configserver
  - .envoy
  - .gateway
  - .stats
  - .push-client
