# logreader

**Значение:** Недоставка логов в агрегаторы
**Воздействие:** Не увидим логи за этот период
**Что делать:**
1. Прихранить выхлоп статручки log-reader'а: `curl -s --max-time 5 'localhost:6180/metrics' | tee logreader.stat.json` (там потом можно посмотреть в value у message-delay)
1. Посмотреть в `/var/log/yc-log-reader/reader.log`
1. Рестартовать `yc-log-reader`
1. Подвязаться к [тикету](https://st.yandex-team.ru/CLOUD-16938).
Помочь может [mudrykaa@](https://staff.yandex-team.ru/mudrykaa)]
