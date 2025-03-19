
certificates:
  source    : 'certs'
  contents  :
    {% if grains['yandex-environment'] == 'testing' %}
    nscfg.mds.yandex.net.key: {{ salt.yav.get('sec-01g54eh1yfga5jxmkg051zs1zv[7F001D2F7B8E8E72483D8C2D2F0002001D2F7B_private_key]') | json }}
    nscfg.mds.yandex.net.pem: {{ salt.yav.get('sec-01g54eh1yfga5jxmkg051zs1zv[7F001D2F7B8E8E72483D8C2D2F0002001D2F7B_certificate]') | json }}
    # Note: currently it's not possible to set separate owners for following certificates.
    d.mdst.yandex.net.key: {{ salt.yav.get('sec-01g54dnhsd1zpb8g9hrkbwpxbx[7F001D2F70914C663C53A9065A0002001D2F70_private_key]') | json }}
    d.mdst.yandex.net.pem: {{ salt.yav.get('sec-01g54dnhsd1zpb8g9hrkbwpxbx[7F001D2F70914C663C53A9065A0002001D2F70_certificate]') | json }}
    {% else %}
    nscfg.mds.yandex.net.key: {{ salt.yav.get('sec-01g54dnf7akg3r6epj11j7q62t[7F001D2F6F7308238E86569E300002001D2F6F_private_key]') | json }}
    nscfg.mds.yandex.net.pem: {{ salt.yav.get('sec-01g54dnf7akg3r6epj11j7q62t[7F001D2F6F7308238E86569E300002001D2F6F_certificate]') | json }}
    d.mds.yandex.net.key: {{ salt.yav.get('sec-01g6nay06w08sd14rjh8xnt5fa[7F001D7D97134E9ABEFBB72A100002001D7D97_private_key]') | json }}
    d.mds.yandex.net.pem: {{ salt.yav.get('sec-01g6nay06w08sd14rjh8xnt5fa[7F001D7D97134E9ABEFBB72A100002001D7D97_certificate]') | json }}
    {% endif %}
  packages  : ['nginx']
  services  : 'nginx'
  check_pkg : 'config-monrun-cert-check'
  path      : "/etc/yandex-certs/"
