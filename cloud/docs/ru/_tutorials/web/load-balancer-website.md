# Отказоустойчивый сайт с балансировкой нагрузки с помощью {{ network-load-balancer-full-name }}

{% if product == "cloud-il" %}

{% include [one-az-disclaimer](../../_includes/overview/one-az-disclaimer.md) %}

{% endif %}

Создайте и настройте веб-сайт на стеке [LAMP]{% if lang == "ru" %}(https://ru.wikipedia.org/wiki/LAMP){% endif %}{% if lang == "en" %}(https://en.wikipedia.org/wiki/LAMP_(software_bundle)){% endif %} ([Linux](https://www.linux.org/), [Apache HTTP Server](https://httpd.apache.org/), [MySQL](https://www.mysql.com/), [PHP](https://www.php.net/)) или LEMP (веб-сервер Apache заменяется на [Nginx](https://www.nginx.com/)) с балансировкой нагрузки через [{{ network-load-balancer-short-name }}](../../network-load-balancer/concepts/index.md) между двумя зонами доступности, защищенный от сбоев в одной зоне.

1. [Подготовьте облако к работе](#before-you-begin).
1. [Подготовьте сетевую инфраструктуру](#prepare-network).
1. [Создайте группу ВМ](#create-vms).
1. [Загрузите файлы веб-сайта](#upload-files).
1. [Создайте сетевой балансировщик](#create-load-balancer).
1. [Протестируйте отказоустойчивость](#test-availability).

Если сайт вам больше не нужен, [удалите все используемые им ресурсы](#clear-out).

## Подготовьте облако к работе {#before-you-begin}

{% include [before-you-begin](../_tutorials_includes/before-you-begin.md) %}

{% if product == "yandex-cloud" %}

### Необходимые платные ресурсы {#paid-resources}

В стоимость поддержки веб-сайта входит:

* плата за диски и постоянно запущенные виртуальные машины (см. [тарифы {{ compute-full-name }}](../../compute/pricing.md));
* плата за использование динамических внешних IP-адресов (см. [тарифы {{ vpc-full-name }}](../../vpc/pricing.md));
* плата за сетевые балансировщики и балансировку трафика (см. [тарифы {{ network-load-balancer-full-name}}](../../network-load-balancer/pricing.md)).

{% endif %}

## Подготовьте сетевую инфраструктуру {#prepare-network}

Перед тем, как создавать ВМ:

1. Перейдите в [консоль управления]({{ link-console-main }}) {{ yandex-cloud }} и выберите каталог, в котором будете выполнять операции.

1. Убедитесь, что в выбранном каталоге есть сеть с подсетями в зонах доступности `{{ region-id }}-a` и `{{ region-id }}-b`. Для этого на странице каталога выберите сервис **{{ vpc-name }}**. Если нужной [сети](../../vpc/operations/network-create.md) или [подсетей](../../vpc/operations/subnet-create.md) нет, создайте их.

## Создайте группу ВМ {#create-vms}

Чтобы создать группу ВМ с предустановленным веб-сервером:

1. В [консоли управления]({{ link-console-main }}) откройте сервис **{{ compute-name }}**.
1. Откройте вкладку **Группы виртуальных машин** и нажмите кнопку **Создать группу**.
1. В блоке **Базовые параметры**:
   * Введите имя группы ВМ, например `nlb-vm-group`.
   * Выберите [сервисный аккаунт](../../iam/concepts/users/service-accounts.md) из списка или создайте новый. Чтобы иметь возможность создавать, обновлять и удалять ВМ в группе, назначьте сервисному аккаунту роль `editor`. По умолчанию все операции в {{ ig-name }} выполняются от имени сервисного аккаунта.

1. В блоке **Распределение** выберите зоны доступности `{{ region-id }}-a` и `{{ region-id }}-b`, чтобы обеспечить отказоустойчивость хостинга.
1. В блоке **Шаблон виртуальной машины** нажмите кнопку **Задать** и укажите конфигурацию базовой ВМ:
   * В блоке **Базовые параметры** введите **Описание** шаблона.
   * В блоке **Выбор образа/загрузочного диска** откройте вкладку **Cloud Marketplace** и нажмите кнопку **Посмотреть больше**. Выберите продукт:
     * [LEMP](/marketplace/products/yc/lemp) для Linux, nginx, MySQL, PHP;
     * [LAMP](/marketplace/products/yc/lamp) для Linux, Apache, MySQL, PHP.
     
     Нажмите кнопку **Использовать**.
   * В блоке **Диски** укажите:
     * **Тип** диска — HDD.
     * **Размер** — 3 ГБ.
   * В блоке **Вычислительные ресурсы** укажите:
     * **Платформа** — Intel Ice Lake.
     * **vCPU** — 2.
     * **Гарантированная доля vCPU** — 20%.
     * **RAM** — 1 ГБ.
   * В блоке **Сетевые настройки**:
     * Выберите облачную сеть и ее подсети.
     * В поле **Публичный адрес** выберите **Автоматически**.
   * В блоке **Доступ** укажите данные для доступа на виртуальную машину:
     * В поле **Сервисный аккаунт** выберите сервисный аккаунт для привязки к ВМ.
     * В поле **Логин** введите имя пользователя.
     * В поле **SSH-ключ** вставьте содержимое файла открытого ключа.  
     Для подключения по SSH необходимо создать пару ключей. Подробнее в разделе [{#T}](../../compute/operations/vm-connect/ssh.md#creating-ssh-keys).    
   * Нажмите кнопку **Сохранить**.

1. В блоке **Масштабирование** укажите **Размер** группы ВМ — 2.
1. В блоке **Интеграция с Load Balancer** выберите опцию **Создать целевую группу** и укажите имя группы: `nlb-tg`.
1. Нажмите кнопку **Создать**.

Создание группы ВМ может занять несколько минут. Когда все ВМ перейдут в статус `RUNNING`, вы можете [загрузить на них файлы веб-сайта](#upload-files).

#### См. также

* [{#T}](../../compute/operations/vm-connect/ssh.md)

## Загрузите файлы веб-сайта {#upload-files}

Чтобы проверить работу веб-сервера, необходимо загрузить файлы сайта на каждую ВМ. Для примера вы можете использовать файл `index.html` из [архива](https://storage.yandexcloud.net/doc-files/index.html.zip).

Для каждой виртуальной машины в [созданной группе](#create-vms) выполните следующее:

1. На вкладке **Виртуальные машины** нажмите на имя нужной ВМ в списке. В блоке **Сеть** найдите публичный IP-адрес.
1. [Подключитесь](../../compute/operations/vm-connect/ssh.md) к ВМ по протоколу SSH.
1. Выдайте права на запись для вашего пользователя на директорию `/var/www/html`:
     
   ```bash
   sudo chown -R "$USER":www-data /var/www/html
   ```

1. Загрузите на ВМ файлы веб-сайта с помощью [протокола SCP]{% if lang == "ru" %}(https://ru.wikipedia.org/wiki/SCP){% endif %}{% if lang == "en" %}(https://en.wikipedia.org/wiki/Secure_copy_protocol){% endif %}.

   {% list tabs %}

   - Linux/macOS

     Используйте утилиту командной строки `scp`:

     ```bash
     scp -r <путь до директории с файлами> <имя пользователя ВМ>@<IP-адрес виртуальной машины>:/var/www/html
     ```

   - Windows

     С помощью программы [WinSCP](https://winscp.net/eng/download.php) скопируйте локальную директорию с файлами в директорию `/var/www/html` на ВМ.

   {% endlist %}

## Создайте сетевой балансировщик {#create-load-balancer}

При создании сетевого балансировщика нужно добавить обработчик, на котором балансировщик будет принимать трафик, подключить целевую группу, созданную вместе с группой ВМ, и настроить в ней проверку состояния ресурсов.

Чтобы создать сетевой балансировщик:

1. В [консоли управления]({{ link-console-main }}) откройте раздел **{{ network-load-balancer-short-name }}**.
1. Нажмите кнопку **Создать сетевой балансировщик**.
1. Задайте имя балансировщика, например `nlb-1`.
1. В блоке **Обработчики** нажмите кнопку **Добавить обработчик** и укажите параметры:
    * **Имя обработчика** — `nlb-listener`.
    * **Порт** — `80`.
    * **Целевой порт** — `80`.

1. Нажмите кнопку **Добавить**.
1. В блоке **Целевые группы**:
    1. Нажмите кнопку **Добавить целевую группу** и выберите [созданную ранее](#create-vms) целевую группу `nlb-tg`. Если группа одна, она будет выбрана автоматически.
    1. В блоке **Проверка состояния** нажмите кнопку **Настроить** и измените параметры:
        * **Имя** проверки — `health-check-1`.
        * **Порог работоспособности** — количество успешных проверок, после которого виртуальная машина будет считаться готовой к приему трафика: `5`.
        * **Порог неработоспособности** — количество проваленных проверок, после которого на виртуальную машину перестанет подаваться трафик: `5`.
    1. Нажмите кнопку **Применить**.

1. Нажмите кнопку **Создать**.

## Протестируйте отказоустойчивость {#test-availability}

1. В [консоли управления]({{ link-console-main }}) откройте сервис **{{ compute-name }}**.
1. Перейдите на страницу ВМ из созданной ранее группы. В блоке **Сеть** найдите публичный IP-адрес.
1. [Подключитесь](../../compute/operations/vm-connect/ssh.md#vm-connect) к ВМ по протоколу SSH.
1. Остановите веб-сервис, чтобы сымитировать сбой в работе веб-сервера:

   {% list tabs %}

   - LAMP

     ```bash
     sudo service apache2 stop
     ```

   - LEMP

     ```bash
     sudo service nginx stop
     ```
   {% endlist %}

1. Перейдите в раздел **{{ network-load-balancer-name }}** и выберите созданный ранее балансировщик `nlb-1`.
1. В блоке **Обработчики** найдите IP-адрес обработчика. Откройте сайт в браузере, используя адрес обработчика. 

   Несмотря на сбой в работе одного из веб-серверов, подключение должно пройти успешно.
1. После завершения проверки снова запустите веб-сервис:

   {% list tabs %}

   - LAMP

     ```bash
     sudo service apache2 start
     ```

   - LEMP
     ```bash
     sudo service nginx start
     ```

   {% endlist %}

## Как удалить созданные ресурсы {#clear-out}

Чтобы остановить работу хостинга и перестать платить за созданные ресурсы:

1. [Удалите](../../network-load-balancer/operations/load-balancer-delete.md) сетевой балансировщик `nlb-1`.

1. [Удалите](../../compute/operations/instance-groups/delete.md) группу виртуальных машин `nlb-vm-group`.
