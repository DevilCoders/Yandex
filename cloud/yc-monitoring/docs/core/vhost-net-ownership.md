[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dvhost-net-ownership)

## vhost-net-ownership

Сигнализирует что на ноде слетели права на /dev/vhost-net.

## Подробности

При использовании vhost-net в виртуалках оказалось что qemu тихо ругается в лог если устройство /dev/vhost-net недступно и включает конфигурацию сети без vhost, что приводит к заметной деградации производительности [CLOUDINC-1899](https://st.yandex-team.ru/CLOUDINC-1899).

Для того чтобы qemu могла использовать устройство на нём должны быть права 0660 и владельцы root:kvm. По умолчанию владельцем устройства является root:root. Правильны группа назначается udev-правилом, но если его применение по какой-то причине завершилось с ошибкой мы получаем ноду, которая не может в vhost-net и об этом никто не знает пока клиент не заметит проблемы.

## Диагностика

В деталях проверки нода на которой неправильные права на устройство.

## Что делать

- Починить права на устройство `sudo chown root:kvm /dev/vhost-net`
- Проверить что уже запущенные машины не летят с медленной сетью (в логе qemu сообщения вида 'warning: tap: open vhost char device failed: Permission denied')
- Расследовать причину поломки прав

## Ссылки
- [Инцидент CLOUDINC-1899](https://st.yandex-team.ru/CLOUDINC-1899)
- [Исправление CLOUD-81273](https://st.yandex-team.ru/CLOUD-81273)
- [Хотфикс CLOUDOPS-5419](https://st.yandex-team.ru/CLOUDOPS-5419)
