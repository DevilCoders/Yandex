[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=compute+node+accounting+errors), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-node-accounting-errors)

## compute-node-accounting-errors
Загорается при логических ошибках в аккаунтинге инстансов (например, попытка включить аккаунтинг, когда процесс гипервизора не запущен).

## Подробности
При таких ошибках отбрасывается текущий Accounting State, чтобы ни в коем случае не перебиллить пользователя (лучше недобиллить).

Ошибки сгруппированы по причинам (метка `reason`). Реализован только в Go Compute Node.

## Диагностика

- Заходим на проблемную compute-node. Например, `pssh myt1-ct5-1.cloud.yandex.net`.
- Ищем ошибки в логах: `sudo journalctl -u yc-compute-node --since -1hour | egrep "accounting.*E:" -B10`
- Анализируем логи, предшествующие ошибке.
- Заводим тикет, прикладываем туда логи. Призываем Дмитрия Симонова.

## Ссылки
- [Тикет на мониторинг](https://st.yandex-team.ru/CLOUD-65496)
- Типы (`reason`) ошибок: [accounting/errors.go](https://bb.yandex-team.ru/projects/CLOUD/repos/compute/browse/go/node/internal/accounting/errors.go)
- Код аккаунтинга: [accounting/accounting.go](https://bb.yandex-team.ru/projects/CLOUD/repos/compute/browse/go/node/internal/accounting/accounting.go)
