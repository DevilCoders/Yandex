sec-tvm: {{salt.yav.get('sec-01g0fhjw77c9tn17gmzwmtyv29')|json}}
sec-pg:  {{salt.yav.get('sec-01g3bwpcb63sd0wk4643ewhj32')|json}}
sec-crt: {{salt.yav.get('sec-01g12ym0bpfzg07vrp66syn8e2')|json}}

bind_networks:
  lab-vla-srv1: 2a02:6b8:c0e:1002:0:675:fff1::/112
  lab-vla-srv2: 2a02:6b8:c0e:1003:0:675:fff2::/112
  lab-vla-srv3: 2a02:6b8:c0e:1003:0:675:fff3::/112
  lab-vla-srv4: 2a02:6b8:c0e:1003:0:675:fff4::/112
