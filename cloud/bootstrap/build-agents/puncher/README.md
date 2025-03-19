# build-agents puncher rule

## How to add rule
### Requisites
* [puncher cli](https://wiki.yandex-team.ru/noc/nocdev/puncher/konsolnyjj-klient/)
### Steps
1. Add a new rule to [file](./_CLOUD_BUILDAGENTS_PREPROD_NETS_.rules.jsonl)
2. Submit a request:
```shell
tail -n1 _CLOUD_BUILDAGENTS_PREPROD_NETS_.rules.jsonl | puncher create --multiple
```
3. Don't forget to commit and make a PR for your changes
