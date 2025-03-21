Основной целью написания этого безумно длинного текста было создание максимально полной, точной и актуальной
документации по сервисам IAM и Resource Manager и в целом процессу разработки новых сервисов в Облаке и их интеграции
с подсистемами аутентификации, авторизации и управления ресурсами. Этот документ является одновременно учебным пособием,
руководством и справочником по сервисам Identity & Access Management (IAM) и Resource Manager.


## Структура

Документ состоит из пяти частей.

Часть 1 представляет собой введение в сервисы IAM и Resource Manager, приводится обзор основных возможностей сервисов и
их назначения.

Вторая часть подробно рассказывает про каждую из подсистем авторизации, аутентификации и управления ресурсами.
Рассматриваются вопросы межсервисного взаимодействия, удаления и блокировки ресурсов. Отдельная глава посвящена
разработке клиентских приложений и организации их доступа к ресурсам клиентов в Облаке. Описываются подходы к
проектированию UI сервисов и его связь с облачными системами аутентификации и авторизации.

В третьей части дано описание процесса разливки нового стенда Облака и установки платформенного сервиса на существующий
стенд. Даются рекомендации по созданию системных ресурсов сервиса и назначению прав системным аккаунтам.

Четвертая часть относится к внутренней инсталляции Облака и будет полезна сервисам, представленным не только как сервисы,
доступные внешним клиентам Облака, но и как запущенные внутри Яндекса для его сотрудников.

Пятая содержит подробные сведения об архитектуре и внутреннем устройстве каждого из сервисов IAM и Resource Manager, и
больше предназначена для разработчиков сервисов IAM и Resource Manager.


## Как пользоваться документом

Если вы никогда не имели дела с разработкой новых сервисов в Облаке (и около него), то найдете здесь полноценный гайд,
как создать такой сервис с нуля. В этом случае имеет смысл прочитать его по порядку целиком хотя бы один раз. В тексте
много примеров и ссылок на реализации сервисов Облака.

Действующие разработчики сервисов Облака найдут здесь более детальные справочные материалы, особенности внутренней
реализации, примеры и наилучшие практики работы с сервисами IAM и Resource Manager, шаблоны проектирования подсистем
аутентификации и авторизации внутри сервиса, советы по оптимизации производительности. Разделы документа максимально не
зависят друг от друга, читать их можно в любом порядке. Также советуем обратить внимание на Changelog и периодические
дайджесты IAM и Resource Manager в [клубе Облака](https://clubs.at.yandex-team.ru/ycp/).

Документ будет полезен и разработчикам сервисов IAM и Resource Manager, как полное и точное описание архитектуры и
реализации сервисов, причин по которым были приняты те или иные решения при их разработке.


## Соглашения об оформлении

{% note alert %}

Нужно обратить внимание на важное ограничение или баг, присутствующий в сервисе на текущий момент.

{% endnote %}


{% note tip %}

Пояснение текущего положения дел, историческая справка, задачи на улучшение.

{% endnote %}


{% note info %}

Дополнительная информация, ссылки на стандарты.

{% endnote %}
