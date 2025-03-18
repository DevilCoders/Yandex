blackbox
========

Клиентская библиотека для работы с Черным ящиком (aka ЧЯ или Blackbox). Состоит из двух частей:
  - `a.yandex-team.ru/library/go/yandex/blackbox` - определяет интерфейс для работы с ЧЯ, базовые запросы, ответы, доступные аттритубы пользователя и т.д.
  - `a.yandex-team.ru/library/go/yandex/blackbox/httpbb` - реализация интерфейса

## Полезности
  - официальная документация к API ЧЯ: https://docs.yandex-team.ru/blackbox/concepts/overview
  - [пример приложения](example-app/main.go)
  - [blackbox godoc](https://godoc.yandex-team.ru/pkg/a.yandex-team.ru/library/go/yandex/blackbox)
  - [blackbox/httpbb godoc](https://godoc.yandex-team.ru/pkg/a.yandex-team.ru/library/go/yandex/blackbox/httpbb)

## TODO
  - поддержать менее распространенные методы: [login](https://docs.yandex-team.ru/blackbox/methods/login), [lcookie](https://docs.yandex-team.ru/blackbox/methods/lcookie) и [user_ticket](https://docs.yandex-team.ru/blackbox/methods/user_ticket)
  - добавить работу с `phone_attributes` и кармой
 