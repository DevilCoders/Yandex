sec: {{salt.yav.get('sec-01g4hekqrevfgzsv17825jg98d')|json}}
include:
  - units.k8s.secrets.ingress-wildcard-crt
  - units.k8s.secrets.ingress-tst-wildcard-crt
