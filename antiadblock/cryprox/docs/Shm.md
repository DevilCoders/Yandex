## Тестирование в Sandbox (ШМ)
[Код в аркадии](https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/antiadblock/qa)

### Логгирование запросов и ответов
Для того, чтобы включить полное логгирование для ШМ нужно для секции NGINX в Nanny использовать переменную окружения PROBABILITY_SEED.  
Сейчас логгируем 1 из 10000 запросов в SAS (PROBABILITY_SEED=10000).  
Логи пишутся на HAHN и хранятся 14 дней:
1. [antiadb-nginx-request-log](https://yt.yandex-team.ru/hahn/navigation?path=//home/logfeller/logs/antiadb-nginx-request-log&)
1. [antiadb-cryprox-response-log](https://yt.yandex-team.ru/hahn/navigation?path=//home/logfeller/logs/antiadb-cryprox-response-log&)    


### Генерация патронов
1. Из логов каждый день sandbox-таска ANTIADBLOCK_GENERATE_AMMO_STUB_TABLE_BIN 
([код](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/tasks/generate_ammo_stub_table/__main__.py), [шедулер](https://sandbox.yandex-team.ru/scheduler/21923/view)) 
генерирует 2 таблицы в YT: [для нагрузочных тестов](https://yt.yandex-team.ru/hahn/navigation?path=//home/antiadb/ammo_stubs/ammo_stubs_cryprox_load_table&), 
[для функциональных тестов](https://yt.yandex-team.ru/hahn/navigation?path=//home/antiadb/ammo_stubs/ammo_stubs_cryprox_func_table&)
которые будут использованы для генерации патронов и стабы
1. Далее sandbox-таска ANTIADBLOCK_BUILD_AMMO_AND_STUBS_FROM_YT 
([код](https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/antiadblock/qa/tasks/build_ammo_and_stubs/__init__.py), [шедулер](https://sandbox.yandex-team.ru/scheduler/22071/view)) 
генерирует патроны для стрельбы

#### Особенности таски ANTIADBLOCK_BUILD_AMMO_AND_STUBS_FROM_YT
1. Переиспользуем код БК, только переопределяем тип output ресурсов
1. Параметр key_header всегда должен быть X-Aab-Requestid, стаба будет использовать этот заголовок, чтобы отдавать контент
1. Параметр testenv_switch_trigger опеределяет, переключать ли Testenv на созданные ресурсы
1. Параметр input_tables - это словарь, который определяет расположение таблицы, из которой генерируются патроны. 
Одновременно можно генерировать несколько пар ресурсов патроны/стаба, используя несколько ключей в словаре. 
Используются ключи cryprox_load и cryprox_func для генерации отдельных пар ресурсов.
1. Параметр spec - это словарь, определяющий спецификацию создаваемых ресурсов. 
Ключи должны совпадать с ключами параметра input_tables. 
Нам по сути нужна настройка "make_load": true, чтобы в качестве патронов создавался ресурс ANTIADBLOCK_DOLBILKA_PLAN
1. [Пример таски](https://sandbox.yandex-team.ru/task/735901235/view)
1. Таска создает пару ресурсов для каждого ключа из input_tables, которые нужно использовать для стрельб совместно 
ANTIADBLOCK_DOLBILKA_PLAN - план запросов, ANTIADBLOCK_CACHE_DAEMON_STUB_DATA - стаба с ответами для кэш-демона

### Общие особенности для всех типов тестирования
1. Используется кастомный [LXC-контейнер](https://sandbox.yandex-team.ru/resource/1515048650/view), 
[таска для сборки](https://sandbox.yandex-team.ru/task/679838152/view)
1. Используется [бинарь NGINX](https://sandbox.yandex-team.ru/resource/1447199501/view)
1. Тестируются две сборки приложения (cryprox + nginx)
1. Фейлим таску, если хотя бы в одной стрельбе много ошибок (отдельно считаем 4xx и 5xx), 
граница определяется пареметром error_rate_thr
1. Таски можно запустить вручную на двух любых сборках cryprox, заполнив все необходимые параметры. 
В качестве сборок прокси можно использовать как собранные [джобой Testenv](https://testenv.yandex-team.ru/?screen=job_history&database=antiadblock&job_name=PACKAGE_CRYPROX_FOR_SANDBOX), 
так и собранные [самостоятельно](#cамостоятельная-сборка-PACKAGE_CRYPROX_FOR_SANDBOX).
1. [Джобы в Testenv](https://testenv.yandex-team.ru/?screen=jobs&database=antiadblock)
1. Во всех стрельбах сохраняются ресурсы с логами приложения для каждого запуска: 
ANTIADBLOCK_CRYPROX_LOG, ANTIADBLOCK_NGINX_ERROR_LOG, ANTIADBLOCK_NGINX_ACCESS_LOG. 
Логи могут помочь при исследовании причины увеличения количества ошибок (4xx или 5xx)
1. Стрельбы умеют брать конфиги не только с продовой инсталляции `configs_api`, но в принципе с любой.
Для этого нужно заполнить параметры в разделе `Configs stub settings`
`configs_api_host` - хост инсталляции `configs_api` (`api.aabadmin.yandex.ru` - production, `preprod.aabadmin.yandex.ru` - preprod, `test.aabadmin.yandex.ru` - для dev-стендов)
`configs_api_tvm_id` - tvm id соотвтествующей инсталляции (2000629 - production, 2000627 - все остальные)

#### Самостоятельная сборка PACKAGE_CRYPROX_FOR_SANDBOX
В корне аркадии выполняем следующие команды:
1. ya package --checkout --target-platform default-linux-x86_64 antiadblock/cryprox/sandbox/package_for_sandbox.json
2. ya upload --ttl 2 -T=ANTIADBLOCK_CRYPROX_PACKAGE  --arch=linux --owner=ANTIADBLOCK cryprox-package-for-sandbox.master.tar.gz 

### Performance тестирование
В данном тестировании проверяем изменение latency и capacity  
**Общие особенности**:
1. Параметр shoot_request_limit определяет количество запросов из плана, которые будут использованы для стрельбы
1. Параметр count_shoot_sessions определяет количество запусков стрельбы для каждой сборки (по дефолту 5)
1. Перед каждой сессией делаем прогрев (желательно пройти весь план запросов, то есть `warmup_rps*warmup_time = shoot_request_limit`)
1. Используются [зависимые автообновляемые ресурсы Testenv](https://testenv.yandex-team.ru/?screen=resources&database=antiadblock), 
[ANTIADBLOCK_CACHE_DAEMON_STUB_DATA_CRYPROX_LOAD](https://a.yandex-team.ru/arc/trunk/arcadia/testenv/jobs/antiadblock/resources.yaml?rev=7039329#L2), 
[ANTIADBLOCK_DOLBILKA_PLAN_CRYPROX_LOAD](https://a.yandex-team.ru/arc/trunk/arcadia/testenv/jobs/antiadblock/resources.yaml?rev=7039329#L10)
1. Дифф запуска можно посмотреть на странице `SHOOTS_DIFF` в таске, также дифф сохраняется в ресурсе типа ANTIADBLOCK_PERFORMANCE_DIFF
1. Статистика performance тестирования пушится в [отчет на Стате](https://stat.yandex-team.ru/AntiAdblock/cryprox_sandbox_tests)


#### Latency (таска ANTIADBLOCK_PERFORMANCE_SHOOTS_DIFF)
1. [Код таски](https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/antiadblock/qa/tasks/performance_shoot_diff/__init__.py)
1. Стрельба проводится в режиме [rps-fixed](https://wiki.yandex-team.ru/JandeksPoisk/Sepe/Dolbilka/#planrps-fixed)
1. Параметры rps, shoot_time собственно определяют настройки стрельбы (желательно, чтобы `rps*shoot_time = 2*shoot_request_limit`)
1. После выполнения таски генерируется отчет, [пример](https://sandbox.yandex-team.ru/task/708245821/report)
1. Для опеределения диффа сравниваем 99 персентиль (по логам NGINX). 
Если медиана для второй сборки хуже медианы для первой сборки (граница определяется параметром perf_diff_thr, по дефолту 2%), 
параметр has_diff выставляем в True
1. Таска поднимается Testenv'ом на каждый коммит. 
[Конфиг](https://a.yandex-team.ru/arc/trunk/arcadia/testenv/jobs/antiadblock/AntiadblockPerformanceShootsDiff.yaml)
1. Все запуски дифф-таски в Testenv с ссылками на Sandbox-таски можно посмотреть на [этой странице](https://testenv.yandex-team.ru/?screen=job_diffs&database=antiadblock&job_name=ANTIADBLOCK_PERFORMANCE_SHOOTS_DIFF)  

#### Capacity (таска ANTIADBLOCK_DETECT_CAPACITY_SHOOT)
1. [Код таски](https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/antiadblock/qa/tasks/detect_capacity/__init__.py)
1. Стрельба проводится в режиме [finger](https://wiki.yandex-team.ru/JandeksPoisk/Sepe/Dolbilka/#finger)
1. После выполнения таски генерируется отчет, [пример](https://sandbox.yandex-team.ru/task/710174662/report)
1. Для опеределения диффа сравниваем получившийся RPS (по дампу долбилы). 
Если медиана для второй сборки хуже медианы для первой сборки (граница определяется параметром rps_diff_thr, по дефолту 5%), 
параметр has_diff выставляем в True
1. Таска поднимается Testenv'ом на каждый коммит. 
[Конфиг](https://a.yandex-team.ru/arc/trunk/arcadia/testenv/jobs/antiadblock/AntiadblockDetectCapacityShoot.yaml)
1. Все запуски дифф-таски в Testenv с ссылками на Sandbox-таски можно посмотреть на [этой странице](https://testenv.yandex-team.ru/?screen=job_diffs&database=antiadblock&job_name=ANTIADBLOCK_DETECT_CAPACITY_SHOOT)    

### Функциональное тестирование (таска ANTIADBLOCK_FUNC_SHOOTS_DIFF)
1. [Код таски](https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/antiadblock/qa/tasks/functional_shoot_diff/__init__.py)
1. Параметры для настройки стрельбы:
    1. `mode=finger`, [режим стрельбы](https://wiki.yandex-team.ru/JandeksPoisk/Sepe/Dolbilka/#finger)
    1. `shoot_threads=1`, для стрельбы используем один поток d-executor (иначе может возникнуть дифф из-за гонки попадания контента в локальный кэш)
    1. `shoot_request_limit`, количество запросов должно быть не больше чем патронов в стабе, на выходных собирается меньше патронов, рекомендуемое значение 50000
1. CRYPROX запускаем в однопроцессном режиме, иначе возникнуть проблемы при записи больших ответов в логи
1. Используются [зависимые автообновляемые ресурсы Testenv](https://testenv.yandex-team.ru/?screen=resources&database=antiadblock), 
[ANTIADBLOCK_CACHE_DAEMON_STUB_DATA_CRYPROX_FUNC](hhttps://a.yandex-team.ru/arc/trunk/arcadia/testenv/jobs/antiadblock/resources.yaml?rev=7152627#L18), 
[ANTIADBLOCK_DOLBILKA_PLAN_CRYPROX_FUNC](https://a.yandex-team.ru/arc/trunk/arcadia/testenv/jobs/antiadblock/resources.yaml?rev=7152627#L26)
1. Для определения диффа сравниваем итоговый request_url, request_tags, зашифрованное тело ответа, код ответа cryprox для каждого запроса
1. Дифф запуска можно посмотреть на странице `SHOOTS_DIFF` в таске, полный дифф сохраняется в ресурсе типа ANTIADBLOCK_FUNC_DIFF (body сохраняется только если дифф был в нем)
1. Таска поднимается Testenv'ом на каждый коммит. 
[Конфиг](https://a.yandex-team.ru/arc/trunk/arcadia/testenv/jobs/antiadblock/AntiadblockFuncShootsDiff.yaml)  
1. Все запуски дифф-таски в Testenv с ссылками на Sandbox-таски можно посмотреть на [этой странице](https://testenv.yandex-team.ru/?screen=job_diffs&database=antiadblock&job_name=ANTIADBLOCK_FUNC_SHOOTS_DIFF)
1. При функциональном тестировании CRYPROX логирует сработавшие регулярки из словаря `REPLACE_BODY_RE`, после отстрела логи парсятся и результат сохраняется
в ресурс типа `ANTIADBLOCK_CRYPROX_REGEX_STAT`

### Тестирование на одной ревизии, но с разными конфигами
1. Таски функционального и перфоманс тестирования умеют запускать отстрелы с произвольными конфигами
1. Необходимо (но это не обязательно) использовать один и тот же пакет с собранным CRYPROX (`cryprox_package_resource`)
1. Необходимо заполнить json в поле `replaced_configs` для каждого движка, необходимо перечислить все заменяемые `label_id` с нужными версиями конфигов,
стаба загрузит нужные конфиги
```json
{
    "zen.yandex.ru": 12617,
    "zen_mobile": 12400,
    "yandex_news": 12583
}
```        

#### Структура диффа в ресурсе ANTIADBLOCK_FUNC_DIFF
1 часть - словарь, который по сути дублирует отчет на странице таски, содержит количество запросов с диффом и без, 
сгруппированных по service_id и request_tags (сохранен как одна строка)    
```json
{
    "without_diff": {
        "liveinternet, PCODE, YANDEX": 2,
        ...
    },
    "with_diff": {
        "yandex.question, YANDEX, BK": 29,
        ...
    }
}
```
2 часть - пустая строка  
3 часть - словарь собственно с диффом (сохранен как одна строка)    
```json
{
    "diff": {
        request_id: {
            "keys": ["request_url", "body"],  # список ключей, для которых найден дифф в запросах
            "0": {"request_url": ["scheme", "netloc", "path", "params", "query", "fragment"], "body": "", "service_id": "", "status_code": 200, "request_tags": []},  # ответ для первого пакета прокси (body сохраняется только если дифф в нем)
            "1": {"request_url": ["scheme", "netloc", "path", "params", "query", "fragment"], "body": "", "service_id": "", "status_code": 200, "request_tags": []},  # ответ для второго пакета прокси
        },
        ...
    },
    "not_exist": {  # список запросов, которые есть в ответах первого пакета прокси, но не во втором (в идеале должен быть всегда пустым)
            request_id: {"request_url": ["scheme", "netloc", "path", "params", "query", "fragment"], "service_id": "", "status_code": 200, "request_tags": []},  # без body
    }
}
```

### Запуск ШМ тасок в прекоммитном тестировании
1. Запуск тасок в прекоммитном тестировании запускается не в основной БД TestEnv, и, соответственно, в основной БД не видны
1. Заходим в свое ревью и кликаем на ссылку RUN в разделе TestEnv Jobs   
![](https://jing.yandex-team.ru/files/dridgerve/shm_1.png)
1. Выбираем необходимые таски и жмем Run Tests   
![](https://jing.yandex-team.ru/files/dridgerve/shm_2.png)
1. Ждем выполнения всех тасок, самая тяжелая ANTIADBLOCK_PERFORMANCE_SHOOTS_DIFF может выполняться до 2-х часов. Выполнение можно отслеживать в таймлайне   
![](https://jing.yandex-team.ru/files/dridgerve/shm_3.png)  
откроется страница   
![](https://jing.yandex-team.ru/files/dridgerve/shm_4.png)  
Статус ОК не означает, что таска выполнилась. Статус дифф-тасок отображается не корректно, корректно отображается только статус тасок PACKAGE_CRYPROX_FOR_SANDBOX
1. Когда таски завершатся, в разделе Success должно быть количество выбранных тасок +1 (таска PACKAGE_CRYPROX_FOR_SANDBOX), либо должны появиться ошибки    
![](https://jing.yandex-team.ru/files/dridgerve/shm_5.png)  
1. Ошибки бывают двух видов: таска показала наличия диффа (можно кликнуть на Diff task и перейти в Sandbox-таску)    
![](https://jing.yandex-team.ru/files/dridgerve/shm_6.png)  
или таска перешла в состоянии FAILURE (превышен порог 4xx или 5xx, либо не обработанные исключения), также есть ссылка на Sandbox-таску    
![](https://jing.yandex-team.ru/files/dridgerve/shm_7.png)
1. Для успешно завершившихся тасок тоже можно перейти в Sandbox    
![](https://jing.yandex-team.ru/files/dridgerve/shm_8.png)
