certificates:
  source    : 'certs-yt'                                 
  files     :                                         
    - 'cert.resize.yandex.net.pem'
    - 'key.resize.yandex.net.pem'

  path      : "/etc/yandex-certs/"                    
  packages  : ['nginx', 'nginx-full', 'nginx-common'] 
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'              
  saltenv: 'stable'
