## Деплой сервиса Антиадблок в облаках

Сервис запущен на [Yp-подах](https://wiki.yandex-team.ru/yp/yp-quick-start-guide/#pod) в среде исполнения взятой из контейнера [registry.yandex.net/antiadb/uni:latest][1], содержащего установленный NGINX и бинарь с собранной Cryprox. Поды разделены на группы по датацентрам и управляются через сервисы Nanny:

production:
    - [cryprox_man](https://nanny.yandex-team.ru/ui/#/services/catalog/cryprox_man/)
    - [cryprox_sas](https://nanny.yandex-team.ru/ui/#/services/catalog/cryprox_sas/)
    - [cryprox_vla](https://nanny.yandex-team.ru/ui/#/services/catalog/cryprox_vla/)
    - [cryprox_myt](https://nanny.yandex-team.ru/ui/#/services/catalog/cryprox_myt/)
    - [cryprox_iva](https://nanny.yandex-team.ru/ui/#/services/catalog/cryprox_iva/)
staging:
    - [test_cryprox_man](https://nanny.yandex-team.ru/ui/#/services/catalog/test_cryprox_man/)
    - [test_cryprox_sas](https://nanny.yandex-team.ru/ui/#/services/catalog/test_cryprox_sas/)
    - [test_cryprox_vla](https://nanny.yandex-team.ru/ui/#/services/catalog/test_cryprox_vla/)

Для просмотра состояния сервисов и централизованного управления используется [дашборд][2].
Для того чтобы изменить версию контейнера, которая используется на Yp-подах, необходимо
1. на панели динамической группировки выбрать `Layers`
1. на панели редактирования версии Docker-образа изменить версию контейнера (кнопка `Edit Layers`) и сохранить, оставив в качестве комментария версию контейнера и ссылку на релизный тикет
1. нажать на кнопку `Deploy` в верху страницы и [выбрать сценарий выкатки][3]
    * для теста `deploy_test` - сценарий с последовательной выкаткой по датацентрам
    * для прода `deploy_prod_fast` - сценарий с последовательным закрытием датацентров и выкаткой на закрытые от балансера поды. Требует ручного подтверждения закрытия каждого из ДЦ, для того чтобы подтвердить нужно нажать на соответсвующем прямоугольнике блок-схемы кнопку в виде треугольника, направленного вправо.
1. нажать на кнопку `Deploy` и проверить в окне подтверждения, что изменяется только версия контейнера
1. дождаться, пока сценарий деплоя закончится - все прямоугольники блок-схемы должны позелениться
1. Время выкатки можно посмотреть на вкладке [Services][4] (Выбрать antiadblock -> cryprox -> cryprox_{dc})


## Выполнение комманд в контейнерах
Для проведения диагностики иногда может потребоваться выполнить комманды на группах хостов. Для этого используется sky, который установлен на хостовых машинах. [Установив себе sky](https://doc.yandex-team.ru/Search/skynet-ag/concepts/gosky-install.html#gosky-install) например на разработческую машину в облаках (если его там еще нет), можно запускать с нее выполнение комманд на всех контейнерах сервиса в nanny (перечисленны выше).

Пример того, как можно проверить работу сети для предоставления инфромации коллегам в RTC-support:
```bash
# сохраняем curl_form из Sandbox-ресурса в persistent-volume
sky portorun -p 'curl -s -o /perm/curl_form.txt https://proxy.sandbox.yandex-team.ru/1200883974' f@test_cryprox_vla
# выполняем 100 запросов из каждого контейнера (на проде рекомендуется перенаправлять вывод в файл добавив к команде "> /tmp/test_cryprox_vla_curl_results") 
sky portorun -p "for i in {1..100}; do curl -w '@/perm/curl_form.txt' -o /dev/null -s 'https://www.livejournal.com/' ; done" f@test_cryprox_vla
```

[1]: https://testenv.yandex-team.ru/?screen=job_history&database=antiadblock&job_name=BUILD_ANTIADBLOCK_UNI_PROXY_DOCKER
[2]: https://nanny.yandex-team.ru/ui/#/services/dashboards/catalog/antiadb_cryrpox/
[3]: https://nanny.yandex-team.ru/ui/#/services/dashboards/catalog/antiadb_cryrpox/deployments/create/
[4]: https://nanny.yandex-team.ru/ui/#/services/