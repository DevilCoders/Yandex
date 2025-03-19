# Утилиты Mr. Prober

Здесь лежат консольные утилиты из мира Mr. Prober

- `yc-mr-prober-synchronize` — синхронизация описаний кластеров и проберов между текстовыми конфигами в аркадии и
  объектами в API. Исходники — в [tools/synchronize/](tools/synchronize/).

- `yc-mr-prober-agent` — вспомогательная утилита для дежурного на агентской виртуалке. Помогает, например, отладить
  проблему, обнаруженную конкретным пробером. Исходники — в [tools/agent/](tools/agent/).

- `yc-mr-prober-meeseeks` — утилита для синхронизации списка компьют-нод в Compute Admin API и в
  переменной `compute_nodes` кластера `meeseeks`. Исходники — в [tools/meeseeks/](tools/meeseeks/).

- `protoc.py` — утилита для обновления протоспек Yandex Cloud. Запускать через `./build_proto_specs.sh` 
в корневой папке Мистера Пробера внутри виртуального окружения.

- `routing_bingo/main.py` - запускатель тестов routing-bingo поверх Mr.Prober API

# Разработка

При разработке консольных утилит мы активно используем библиотеки
[click](https://click.palletsprojects.com/) и [rich](https://github.com/willmcgugan/rich). Примеры можно посмотреть в
существующих утилитах.

Общий для всех утилит код вынесен в `tools/common`. Простейшая утилита выглядит так:

```python
from tools.common import main, cli, console


@cli.command(short_help="welcomes the world")
def hello():
    console.print("[green3]Hello world![/green3]")


if __name__ == "__main__":
    main()
```

# Запуск

В случае настроенного виртуального окружения утилиты запускаются так же, как и другие части проекта

```bash
PYTHONPATH=. tools/<TOOL_NAME>/main.py <ARGUMENTS>
```

Но для удобства использования конечными пользователям утилиты могут быть запакованы в докер-образ и запускаться через
докер-контейнер с помощью bash-обёрток. Пример смотрите в
`tools/synchronize/yc-mr-prober-synchronize`.

Обёртка `yc-mr-prober-agent` устроена немного по-другому: её код доставляется на агентские виртуалки с компьютном
образе. Подробности — в [agent/README.md](agent/README.md).
