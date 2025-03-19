# juggler-sdk генератор проверок
Juggler doc: https://wiki.yandex-team.ru/sm/juggler/

## Setup
Install https://juggler-sdk.n.yandex-team.ru/

`pip install --index-url https://pypi.yandex-team.ru/simple juggler-sdk`

## Workflow

- Modify checks generation in ./juggler.py
- Run generate and dry-run:
```
$ ./juggler.py --env preprod
No-apply (dry run) mode: generate, save file, send dry run (validation) request to server.
Validate changes in 'preprod.json', and then apply changes run command with '--apply' flag.
12 checks written to preprod.json
Changed or created checks:
  cloud_preprod_mk8s-masters:k8s-master-csi-liveness-probe
  cloud_preprod_mk8s-masters:k8s-master-csi-attacher
  cloud_preprod_mk8s-masters:k8s-master-csi-controller
```
- View diff in `(preprod|prod).json`
- If all is ok, then apply changes:
```
$ ./juggler.py --env preprod --apply
You should set environment variable JUGGLER_OAUTH_TOKEN for authorization.
Going to generate checks, save to file, and APPLY.
12 checks written to preprod.json
Changed or created checks:
  cloud_preprod_mk8s-masters:k8s-master-csi-liveness-probe
  cloud_preprod_mk8s-masters:k8s-master-csi-attacher
  cloud_preprod_mk8s-masters:k8s-master-csi-controller
Changes successfully applied!
See result at: https://juggler.yandex-team.ru/aggregate_checks/?query=host%3Dcloud_preprod_mk8s-masters
```

## Мотивация

Следуя [документации juggler](https://wiki.yandex-team.ru/sm/juggler/ecosystem/.edit?section=1&goanchor=h-1)
juggler-sdk это единственный православный инструмент для того, чтобы создавать конфигурации проверок.

juggler-ansible Не рекомендуется к использованию в новых проектах.

**NOTE(skipor):** juggler-ansible has awful user experience. User need to understand how ansible works,
and it is template YAML programming.
Also, it is hard to set up environment for it. I have experience, when after reinstall I can't make it work.

