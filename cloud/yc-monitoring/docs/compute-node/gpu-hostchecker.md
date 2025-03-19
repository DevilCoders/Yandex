[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dgpu-hostchecker)

## gpu-hostchecker
На текущий момент сигнализирует об ошибках PCI Advanced Error Reporting Uncorrectable
Error для видеокарт.

## Подробности
Интерфейс `PCI` представляет из себя шину где передача информации представлена
в виде пакетов. В процессе работы устройств могут происходить ошибки на
различных уровнях: `Transaction` уровне, `Data Link` и `Physical`. Ошибки делятся
на correctable и uncorrectable. Correctable ошибки могут быть устранены на
уровне железа. Здесь мы мониторим только uncorrectable ошибки. Более детально
о самих типах ошибок можно почитать
[здесь](https://www.design-reuse.com/articles/38374/pcie-error-logging-and-handling-on-a-typical-soc.html).
Ошибки делятся на `Fatal` и `Non-Fatal`. Мы проверяем лишь факт того, что
хотя бы один из возможных вариантов ошибок был выставлен в PCIe регистре
`Status`. Другие проверки на текущий момент не включены.

## Диагностика
Для того, чтобы более детально посмотреть какие ошибки возникли следует
зайти на хост и вызывать `sudo lspci -vvvv -s <device-id>`:

    Capabilities: [420 v2] Advanced Error Reporting
    ...
    UESta:	DLP- SDES- TLP- FCP- CmpltTO- CmpltAbrt- UnxCmplt- RxOF- MalfTLP- ECRC- UnsupReq- ACSViol-

Поле `UESta` показывает какие ошибки были зарегистрированы. Подробное описание самих
ошибок можно найти, например, по ссылке выше.
