# transfer activation checker

Функция, которая проверяет активность CDC-трансферов. Если какой-то из них остановлен (state=DONE), активирует.
Также делает deactivate раз в неделю.

Функция существует в Облаке: https://console.cloud.yandex.ru/folders/yc.dwh.service-folder/functions/functions/d4et3eq66kc48c0um42v/overview

Запускается триггером: https://console.cloud.yandex.ru/folders/yc.dwh.service-folder/functions/triggers/a1s7r9llplfa8admj4pe/overview

### Как добавить новый трансфер
Для добавления нового трансфера, нужно прописать id и name в [конфиге](./src/function_config.py)
