### Основной модуль VH

[Deployer CLI](../deployer/README.md) использует этот каталог, как место, где лежат воркфлоу и конфиги к ним.

#### Минимальная структура

* [`config`](сonfig) - основные конфигурационные настройки (имена секретов, окружений, endpoints и т.д.).
    * Основной класс конфига должен наследоваться от `cloud.dwh.nirvana.config.BaseDeployConfig` и называться `DeployConfig`.
    * Имя файла должно совпадать с названием окружения. Deployer будет искать `DeployConfig` в файле из `--env`. Например, [config/preprod](config/preprod.py)
* [`workflows`](workflows) - модули, которые описывают создание workflow и reaction. Подробнее в [README](./workflows/README.md)

#### Дополнительно
* [`common`](common) - общие константы и методы
    * [`operations`](common/operations.py) - константные id операций из Nirvana и их обертки
