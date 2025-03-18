## projects/fintech/verify_pci_dss

Тасклет для проверки PCI DSS статусов коммитов через API Арканума.
Проверяет все коммиты начиная с предыдущего успешного релиза до коммита с которого начали релиз.
В случае первого релиза, когда нет успешного последнего релиза, проверяются коммиты начиная с `initial_revision`.
Все коммиты должны быть в статусах `shipped` или `approved`.
Для работы тасклета нужен токен Арканума, который можно получить здесь: https://docs.yandex-team.ru/arcanum/communication/public-api.

### Конфигурирование

```yaml
verify-job:
  task: projects/fintech/verify_pci_dss
  input:
    config:
      secret_spec:
        uuid: sec-01fcqvw4rd9e1dhvw8f332v1w3 # optional
        key: arcanum
      component: Fintech - Bank Authproxy PCI DSS
      package: taxi/uservices/services/bank-authproxy/package.json # optional
      initial_revision: 9503605 # optional
      abc: fintech_yacloud_cicd
```
