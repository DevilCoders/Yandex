/cloud/dwh/nirvana/vh/common - core-часть фреймворка для обработки данных под управлением nirvana

../config - конфигурация wf
    base -  общая для всех сред

Конфигурируемые параметры:
    environment
    yt_cluster
    yt_token
    yt_owners
    ttl -   время работы операции yql в минутах, определено для yql1 и yql2
            после истечении указанного времени wf падает с ошибкой Command timed out
            значение по умолчанию может быть переопределено для каждой среды и для каждого workflow
    yt_folder
    yql_token
    solomon_token
    solomon_cluster
    nirvana_quota
    mdb_token
    staff_token
    reactor_prefix
    reactor_owner
