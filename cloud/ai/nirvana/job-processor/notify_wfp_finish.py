import requests

instanceId = "1bd77af1-40de-4602-a7f1-5b585a1bae92"

url = "https://wfp.nirvana-deploy.yandex-team.ru/api/processor/callback/v1/statusChanged?instanceId=" + instanceId

json_ = {
  "status": "FINISHED",
  "result": "SUCCESS",
  "progress": 1.0,
  "message": "",
  "details": "\\n\\nLatest events for  :\\n\\n\\tnot available",
  "errorCode": "null",
  "executionTimestamps": {},
  "reusedOutputs": [],
  "issues": []
}

resp = requests.put(url,
                    headers={'Content-Type': 'application/json'},
                    json=json_,
                    verify='/etc/ssl/certs/YandexInternalRootCA.pem')
