# Нагрузочные стрельбы antiadblock-cryprox
Процесс автоматических стрельб состоит из следующих шагов:
1. Генерация стабов - картинки, html с шифрованными ссылками - выполняется в stub_generator.py
2. Генерация патронов - формат, описывающий запросы, которые танк будет отправлять в сервис -
3. Наливка стенда - Ставим заглушечный nginx, который будет выполнять роль партнёра, ставим (прокси + nginx) со специальными настройками, которые будут все запросы редиректить в заглушечный nginx
4. Заливаем патроны и стабы на стенд
5. Говорим танку забирать патроны со стенда и стрелять в него

## Сборка docker-образа nginx-patner-stab

### Создание стабов и патронов
```bash
cd ./stub_generator && \
ya package --checkout --target-platform default-linux-x86_64 --raw-package package.json && \
cd ./stub_generator.bin/ && \
./stub_generator && \
ya upload stubs --tar --owner ANTIADBLOCK --description "stubs for load test" && \
cd ../..
```
### Сборка docker-образа
Полученный id ресурса записываем в `nginx-partner-stub/package.json` и коммитим, чтобы testenv собрал новую версию контейнера registry.yandex.net/antiadb/nginx-partner-stub:latest
