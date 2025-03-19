sec: {{salt.yav.get('sec-01dz3q76qc2342rgya05m8ma73') | json}}
wg_sec: {{ salt.yav.get('sec-01e6gsb01xfzy288h9kg5z7wav') | json}}
sdc_sec: {{ salt.yav.get('sec-01fyxp3bgpavq7nbrznbtkdyn6') | json}}
unified_agent:
  tvm-client-secret: {{ salt.yav.get('sec-01g5r9dqqwpfgn4xdp64x8apay')['client_secret']|json}}
sec_sevensignal: {{salt.yav.get('sec-01g74c654w7tx67j88rnwc8s14') | json}}
