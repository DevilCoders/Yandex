Скрипт captcha_events.sh выделяет из eventlog'а события, касающиеся капчи, 
и преобразуется их в формат, понимаемый программой Logstalgia. 

Результат:
 - Программа выдаёт в СТАНДАРТНЫЙ ВЫВОД строки в формате 
 timestamp|IP|path|response_code|response_size|success|response_colour
 
Примеры использования:
 - captcha_events.sh 20130901 - обработать eventlog за 1 сентября 2013 года
 - captcha_events.sh 20130901-10:00:00 20130901-12:00:00 - обработать данные за период с 10:00 до 12:00 1 сентября 2013 года
 - captcha_events.sh 20130901-10:00:00 - недопустимо
 
Примеры использования совместно с Logstalgia:
 - ./captcha_events.sh 20130901 | ./logstalgia -f -g a,.*,99 -
 - ./captcha_events.sh 20130901 > t.txt ; ./logstalgia -f -g a,.*,99 t.txt
 
При вызове logstalgia рекомендуется использовать параметр "-g a,.*,99", 
чтобы она не добавляла группы "Images", "CSS", "Static" и "MISC."

Весьма неплохо выглядит визуализация, если запускать logstalgia так: 
./logstalgia -f -g a,.*,99 --no-bounce --paddle-position 0.75 --hide-response-code --hide-paddle --glow-duration 0.05 --glow-intensity 0.35 --glow-multiplier 0.5