# Утилита для выкатывания релиза nbs

## Пример запуска

```bash
./release --verbose -c configs/blockstore.yaml -V 86.tags.releases.nbs.stable-20-6-17 -T NBSOPS-1113 -d '20-6-17' --cluster-name testing --zone-name vla
```

Для отправки сообщений в Телеграм, написания комментариев в Стартрек и работы с z2 нужны секреты. Путь к json-файлу с секретами можно передать в параметре ```--secrets```, либо положить в: ```~/.nbs-tools/secrets.json```
