# VINS command line client 
Консольный HTTP-клиент к VINS

# Текстовые запросы, вывод на экран (для ручных запросов)
Если хочется просто задать запрос и увидеть реузльтат, можно воспользоваться опцией ```-s``` aka ```--simple```, чтобы выводить результат в stdout в человекочитаемом виде. Пример:

```
$ echo 'Привет, Алиса!' | vins_client --vins-url <VINS_URL> --bass-url <BASS_URL> -t 2 -s
```

# Текстовые запросы, вывод в файлы (для скриптов)
По умолчанию без опции ```-s``` на входе ожидается поток из строк, содержащих текстовые запросы (одна строка - один запрос), простейшая команда запуска выглядит так:
```
$ cat requests.txt | vins_client --vins-url <VINS_URL> --bass-url <BASS_URL> -t 2 -o ./vins_output
```

Будет создана директория ```./vins_output``` с файлами:
- ```success.requests.txt``` - те входные запросы, которые были обработаны успешно
- ```success.responses.json``` - успешные ответы в виде JSON в том же порядке под одному в строчке (чтобы было удобнее парсить скриптом)
- ```errors.txt``` - ошибочные запросы и тексты ошибок

# Готовые запросы в виде JSON
Если есть файл с запросами в [формате VINS](https://wiki.yandex-team.ru/voicetechnology/dev/voiceinterface/vins/speechkitapi/), их тоже можно использовать если добавить опцию ```-f json``` или ```--input-format json```. Пример:
```
$ cat requests.json | vins_client --vins-url <VINS_URL> --bass-url <BASS_URL> -t 2 -o ./vins_output -f json
```
Сейчас ожидается, что json'ы расположены по одному в строке и \n является разделителем (но это особенности реализации, которые должны исчезнуть с добавлением нормального потокового json-парсера).

# Дополнительные опции
```--patch-file patch.json``` - JSON-патч в формате VINS, который следует добавлять к каждому запросу перед отправкой
```--client``` - удобный shotrcut для предыдущей опции, например ```--client quasar-tv``` добавит минимум полей, чтобы "прикинуться" Колонкой с подключенным телевизором. Список можно пополнять.
```-t/--threads``` - число потоков для обстрела
```--bass-url``` - кастомный урл BASS
```--vins-url``` - кастомный урл VINS
```-e/--exp``` - задать эксперименты (через запятую или повторными вставками -e)
```--oauth``` - добавить в запрос oauth токен
