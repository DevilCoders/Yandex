[Алерт contrail-zookeeper в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-zookeeper)

## Что проверяет

Создает, читает и удаляет тестовую ноду в zookeeper.

## Если загорелось

- `systemctl status zookeeper`

- логи в `/var/log/zookeeper/zookeeper.log`

  если в логах есть `Exception causing close of session 0x0 due to java.io.IOException: ZooKeeperServer not running`, это, возможно [CLOUD-60900](https://st.yandex-team.ru/CLOUD-60900), рестартануть сервис.

- `sudo /usr/share/zookeeper/bin/zkCli.sh ls /` — посмотреть внутрь zookeeper