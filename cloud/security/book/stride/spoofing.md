## Spoofing

Напомню, что Spoofing это в первую очередь про подделку чего-либо, то есть нарушение свойства Authentication (да, spoofing IP это случай, когда у вас аутентификация по IP).

### Уровень сети

#### Угрозы managed сервисов и использования VPC пользователя

Если вы делаете managed севис, то часто вам приходится делать виртуальную машину с двумя сетевыми интерфейсами и один интерфейс подключать к пользовательской VPC, а другой в managed сеть. В этом случае следует помнить о том, что

1. Пользователь управляет настройками DHCP в своей сети и поэтому на ваш интерфейс могут приехать любые DHCP Options. Это может привести к тому, что пользователь может сломать время, может заставить виртуальную машину делать resolve доменов (в том числе инфраструктурных) в подконтрольном ему DNS.
2. Пользователь может управлять маршрутами в этой сети, может попытаться перехватывать трафик, проводить атаки Man-in-the-middle и в конечном итоге пытаться выбраться на виртуальную машину сервиса.

Очень важно

1. Не доверять тому, что приходит через интерфейс пользовательской сети (ни IP-адресам, ни DHCP options, ни чему-либо другому);
2. Проверять корректность TLS сертификатов (ни к в коем случае не отключать проверку), не использовать протоколы без шифрования;
3. Ходить в метаданные (169.254.169.254) через интерфейс в managed сети;
4. Не открывать никаких служебных сервисов в пользовательскую сеть (например, ssh должен слушать только служебный manged интерфейс)
5. Использовать только Cloud DNS для резолва доменов в IP-адреса.

{% note alert %}

При моделировании угроз на диаграмме необходимо обязательно указать сервисы и порты, которые доступны пользователю в его VPC. Необходимо минимизировать количество сервисов, доступных их VPC пользователя, так как это уменьшает поверхность атаки на managed сервис.

{% endnote %}

Примеры инцидентов

* [CLOUDINC-2487](https://st.yandex-team.ru/CLOUDINC-2487)
* [CLOUDINC-2182](https://st.yandex-team.ru/CLOUDINC-2182)

#### Использование Security Groups

**TBD**

### Некорректная аутентификация в IAM

В Облаке в этом пункте мы в первую очередь смотрим на корректную работу по управлению доступом, которая должна выражаться в правильной интеграции с IAM.

{% note alert %}

В Облаке любая аутентификация и авторизация должна осуществляться через IAM. Любые исключения из этого правила требуют явного аппрува команд IAM и Security (в виде принятого риска в очереди `CLOUDRISKS`), а также должно быть отражено во внутренней документации сервиса.

{% endnote %}

Подробно о том, как правильно интегрироваться с IAM можно прочитать в [IAM Cook Book](https://docs.yandex-team.ru/iam-cookbook/).
