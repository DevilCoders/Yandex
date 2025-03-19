# Сервер моделей

Docker образ с сервером моделей. Использует [Tensorflow Serving](https://www.tensorflow.org/serving), наружу торчит REST API. 

## Сборка и запуск

Модели забираются в контейнер из папки `models/`.

Сборка:
```bash
make build
```

Запуск:
```bash
make run
```

Тест:
```bash
make test
```

## Подготовка моделей

Подготовка Badoo модели для Tensorflow Serving:

```bash
model_server$ cd misc/badoo
model_server/misk/badoo$ python prepare_graph_badoo.py
```

В директории рядом со скриптом должен лежать файл с моделью `graph.pb`.

## Нагрузочное тестирование

Сначала генерируем патроны для Яндекс.Танка для стрельб по контейнеру. Патроны генерируются для POST запроса вида
```
curl http://127.0.0.1/v1/classify -X POST -F "data=@image.jpg" -F "model=badoo"
```

Скрипт берет папку `images/` и по ней для каждой картинки генерирует запрос.

```bash
model_server$ cd load/ 
model_server/load$ python ammo_generator.py > ammo
```

Стрельбы проводятся в Qloud окружении https://qloud-ext.yandex-team.ru/projects/junk/o-gulyaev/automl-image-prototype
