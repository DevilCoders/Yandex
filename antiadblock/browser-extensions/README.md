# Ludka decoder

При открытии на активной страницы пытается достать из нее результат детекта ludca

точно работает в Chome/Chromium/YandexBrowser/Opera

Установка:
- зайти chrome://extensions/
- справа вверху включить Developer mode
- содержимое папки как 'Load unpacked' загрузить

Приложение - значок L среди приложений.


# Modify Header

При открытии урла `https://ya.ru/add?name1=value1&name2=value2` добавляются хедеры на все запросы и происходит редирект 
на `https://ya.ru/`.

Если в конце значения хедера будет символ `*`, то на каждый запрос значение хедера будет `value-<count>`. Например 
`value-0, value-1, ..., value-322, ...`

При открытии урла `https://ya.ru/clear` удаляются все хедеры из плагина и происходит редирект на
`https://ya.ru/`

Скачать расширение для Chrome можно через SB ресурс `ANTIADBLOCK_CHROME_EXTENSION_MOD_HEADER`

Или можно на примере `Ludka` через `Load unpacked`.

Скачать расширение для Firefox можно через SB ресурс `ANTIADBLOCK_FIREFOX_EXTENSION_MOD_HEADER`

Собрать `.xpi` формат например можно через панель управления расширениями для Firefox. 
