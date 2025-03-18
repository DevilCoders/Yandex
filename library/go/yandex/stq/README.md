# Адаптер для поддержки go воркеров в STQ

STQ (Sharded Tasks Queue) предоставляет инфраструктуру для асинхронного вызова функции, задачи. Это не очередь задач в прямом смысле, а скорее некий пул задач. К имени очереди привязывается функция-обработчик, которая живёт на воркерах очереди.

[Подробнее про STQ](https://wiki.yandex-team.ru/taxi/backend/architecture/stq)

Go адаптер позволяет обрабатывать STQ задачи кодом, написанным на go.

## Использование

Для подключения адаптера в своём коде, добавьте `"a.yandex-team.ru/library/go/yandex/stq/pkg/worker"` в список импортов.

Чтобы обрабатывать задачи, необходимо реализовать [интерфейс](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yandex/stq/pkg/worker/worker.go)
```
type Worker interface {
	ProcessRequest(request Request) error
}
```

В директории [cmd/example/workers](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yandex/stq/cmd/example/workers) лежат простые примеры воркеров, которые можно взять за основу при написании новых.

## Тестирование

Базовую логику вашего воркера вы можете протестировать, покрыв код go тестами.

Для проверки работы вашего воркера на тестовом окружении на реальных задачах, приходящих с сервера, вы можете по [инструкции](https://wiki.yandex-team.ru/taxi/backend/architecture/stq-runner/manual/#lokalnyjjzapuskstq-runnernavirtualke) задеплоить сервис `stq-runner`, который запустит вашего воркера и будет передавать ему задачи на выполнение.