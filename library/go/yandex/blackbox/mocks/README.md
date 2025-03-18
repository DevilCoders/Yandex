blackbox mocks
====================

Данный пакет содержит mock на основе `github.com/golang/mock/mockgen` для [клиента](../client.go) к ЧЯ.

## Генерация
```bash
cd ${ARCADIA_ROOT}/library/go/yandex/blackbox
mockgen -source=client.go -destination mocks/client.go -package mocks
```

