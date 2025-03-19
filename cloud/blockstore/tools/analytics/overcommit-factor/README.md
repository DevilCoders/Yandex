# Overcommit Factor

Тула для вычисления overcommit-фактора, который впоследствии можно указать в конфиге сервера в поле OvercommitPercentage, для регулирования лимитов клиентского тротлинга.

Порядок действий:
1) Получить список инстансов и их cpu_unit_count:
%%pssh run -p 32 "cat /etc/yc/infra/nbs-throttling.json | jq -r '.compute_cores_num' && sudo yc-compute-node-keyctl list-endpoints | grep -e instance-id -e cpu_unit_count" C@cloud_testing_nbs > testing.txt%%
2) Получить инфу про cores и core-fraction для всех инстансов отсюда https://nda.ya.ru/ вот таким скриптом:
%%SELECT id, resources, status
FROM `/testing_global/ycloud/hardware/default/compute_az/instances`%%
Сохранить результат в cores_testing.txt
3) Получить токен для соломона отсюда https://docs.yandex-team.ru/solomon/api-ref/authentication#oauth
Запустить тулу %%$ ya make -r; env TOKEN='XXX' ./overcommit_factor --cluster testing --instance-file ./testing.txt --cores-file ./cores_testing.txt%% и на выходе будет 99.9 процентиль по overcommit_factor и файлик testing_of.csv, в котором есть overcommit_factor каждого инстанса.

Для препрода и прода все делаем по аналогии. Только в пункте 2 другие ссылки:
- препрод: https://monitoring.ydb.yandex-team.ru/tenant?general=query&name=%2Fpre-prod_global%2Fycloud&backend=https%3A%2F%2Fydb.bastion.cloud.yandex-team.ru%2Fu-vm-cc8hdrhlh39a5165ijk5-ru-central1-b-oswa-uzis.cc8hdrhlh39a5165ijk5.ydb.mdb.cloud-preprod.yandex.net%3A8765&info=overview&clusterName=cloud_preprod_kikimr_global
- прод: https://monitoring.ydb.yandex-team.ru/tenant?schema=%2Fglobal%2Fycloud&general=query&name=%2Fglobal%2Fycloud&backend=https%3A%2F%2Fydb.bastion.cloud.yandex-team.ru%2Fycloud-dn-myt7.svc.cloud.yandex.net%3A8765&info=overview&clusterName=cloud_prod_kikimr_global
