## Разработка cэндбоксных тасок

К сожалению, единой пошаговой инструкции по тому, как создавать виртуалку и как разворачивать сэндбокс нет.

Есть несколько страниц в вики, 

https://wiki.yandex-team.ru/sandbox/allocate-host/

https://wiki.yandex-team.ru/sandbox/environment-setup/

https://wiki.yandex-team.ru/sandbox/quickstart/

в целом хорошо описывающих процесс и интерфейс QYP очень простой.

Есть уже развернутая машина с сэндбоксом

ashaposhnikov-dev-sandbox2.sas.yp-c.yandex.net

Недостатки у использования общего инстанса:
1) он запущен от имени ползьзователя ashaposhnikov, sandbox отказывается стартовать под root
2) единственный известный способ обновить код тасок это поменять его в /home/ashaposhnikov/arcadia 
После этого сервис рестартует. Если есть ошибки, сервис вываливает их в лог и не стартует. Если 2 человека начнут
одновременно писать код, это будет ужасно мешать.

Так что возможно лучше будет завести свою виртуалку.

Разработка тасок проста, но есть особенности:
часто сервер из-за ошибок в тасках ложится и приходится
1) исправлять таску
2) делать arcadia/sandbox/sandbox start

