import subprocess
import json

if __name__ == '__main__':
  new_vs = set(subprocess.check_output('puncher get --destination new-ipv6.dpl01.ycp.cloud.yandex.net --ports 443 --json-request | jq -r .sources[]', shell=True).splitlines())
  new_rs = set(subprocess.check_output('puncher get --destination _CLOUD_L7_PROD_NETS_ --ports 3443 --json-request | jq -r .sources[]', shell=True).splitlines())
  old_rules = subprocess.check_output('puncher get --ports 443 --destination dpl01.ycp.cloud.yandex.net --json-request', shell=True).splitlines()
  for r in old_rules:
    if not new_vs.issuperset(json.loads(r)['sources']):
      print('VS:', r)
    if not new_rs.issuperset(json.loads(r)['sources']):
      print('RS:', r)
