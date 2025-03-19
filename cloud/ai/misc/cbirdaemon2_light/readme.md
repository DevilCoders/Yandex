## Образ

Образ поставляется в виде `.tar` файла. Его необходимо скачивать по адресу 
```
https://storage.yandexcloud.net/badoo/cbirdaemon.tar
```
 
Загрузить его можно с помощью команды
```
docker load < cbirdaemon.tar
```

Запуск образа:
```
docker run -p 80:15522 --rm -d cbirdaemon:latest app/cbirdaemon2_light --port 15522 --threads 10
```

После запуска образ будет в рабочем состоянии через несколько секунд.

Для запуска образа с параметрами по умолчанию используйте команду
```
docker run -P --rm -d cbirdaemon:latest
```

Образ будет запущен с 10 потоками на 15522 порту внутри контейнера

## API

### Запрос

```
curl -i 127.0.0.1/ping
```

Проверка того, что сервер поднят и может отвечать на запросы. В случае успеха возвращает статус 200.

### Ответ

```
HTTP/1.1 200 Ok
Connection: Keep-Alive
Transfer-Encoding: chunked
```

### Запрос

Content-Type: multipart/form-data

```
curl -D - -H "RequestInfo: {\"ImageInfo\":{},\"CbirFeatures\":{\"Quant\":true,\"Dscr\":false}}" -H "Sigrequest: configurable_v2" -F "upfile=@test.jpg" http://localhost
```

Поддерживаются растровые изображения в формате `jpeg/png/gif`

### Ответ

В поле quant возвращается сигнатура картинки, по которой можно искать копии изображения

```json
HTTP/1.1 200 Ok
Content-Length: 4409
Connection: Keep-Alive

info_orig{632x474}
info{632x474}
quant{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}
```

В случае некорректного файла, который не является картинкой, возвращается статус-код 500:
```json
HTTP/1.1 500 Internal server error
Date: Thu, 05 Dec 2019 22:47:35 GMT
Timing-Allow-Origin: *
Content-Type: text/plain
Content-Length: 19
Connection: Keep-Alive
```

## Метод Vision batchAnalyze

Анализирует набор изображений и возвращает результат поиска подобных изображений.

### HTTP-запрос

```
POST https://vision.api.cloud.yandex.net/vision/v1/batchAnalyze
```

### Параметры в теле запроса
```json
{
  "analyzeSpecs": [
    {
      "features": [
        {
          "type": "IMAGE_COPY_SEARCH",
        }
      ],
      "mimeType": "string",
      "signature": "string"
    }
  ]
}
```

`analyzeSpecs[].mimeType` - MIME-тип (https://en.wikipedia.org/wiki/Media_type) контента (например, application/pdf).
Максимальная длина строки в символах — 255.

`analyzeSpecs[].signature` - Сигнатура изображения, получаемая из докер образа.
Максимальная длина строки в символах — 16384.

Параметр `analyzeSpecs[].features[].type` необходимо указать как `IMAGE_COPY_SEARCH`

### Ответ

##### HTTP Code: 200 - OK
```json
{
 "results": [
  {
   "results": [
    {
     "imageCopySearch": {
      "topResults": [
       {
        "imageUrl": "string",
        "pageUrl": "string",
        "title": "string",
        "description": "string"
       },
       // другие элементы списка `results[].results[].imageCopySearch[].topResults[]`
      ],
      "copyCount": "integer"
     }
    }
   ]
  }
 ]
}
```

`results[].results[].imageCopySearch.topResults[]` - Результат работы, список ссылок и сайтов, где встречается подобная картинка, упорядоченный по убыванию релеватности. 
`imageUrl` - URL картинка, по которому она доступна
`pageUrl` - URL страницы, где встречается картинка
`title` - заголовок страница
`description` - описание страницы

`results[].results[].imageCopySearch.copyCount` - Количество найденных копий

### Пример запроса

```
curl -H "Authorization: Api-Key <Ваш Api-Key>" --data @data.json https://vision.api.cloud.yandex.net/vision/v1/batchAnalyze 
```

Файл `data.json` имеет следующее содержание:
```json
{
  "analyzeSpecs": [
    {
      "features": [
        {
          "type": "IMAGE_COPY_SEARCH",
        }
      ],
      "mimeType": "application/json",
      "signature": "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"
    }
  ]
}
```

В результате выполнения получим следующий ответ:

```json
{
 "results": [
  {
   "results": [
    {
     "imageCopySearch": {
      "topResults": [
       {
        "imageUrl": "https://mtdata.ru/u8/photoEEEB/20465719371-0/original.jpg",
        "pageUrl": "www.eg.ru/showbusiness/802497-blondinka-v-dospehah-poyavilos-foto-djoli-so-semok-novogo-filma-marvel/",
        "title": "Анджелина Джоли в фильме Вечные - Экспресс газета",
        "description": "Анджелину сложно узнать в образе воительницы Тены. "
       }
      ],
      "copyCount": "1"
     }
    }
   ]
  }
 ]
}
```
