[Алерт walle_fs_check в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dwalle_fs_check)

Есть ошибки в файловой системе.

Если других проявлений нет (все сервисы работают, нет ошибок в dmesg, ...), следует быть настороже, но срочных действий можно не предпринимать.

Что стоит сделать:

- призвать дежурного NBS и показать ему,

- проверить файловую систему на виртуальной машине, используя [fsck](https://wiki.yandex-team.ru/cloud/compute/duty/#sposobyvosstanovlenijaposlekorrapshenadannyxnanbs) + перезагрузиться (это может как починить машину, так и убить ее окончательно!),

- если не помогае — пересоздать SVM