# ZFP (Zero flap policy)

В этом документе содержится описание процесса ZFP, суть которого сведение к 0 флапов алертов (по аналогии с ZBP, для нас алерт - это программа, флап - баг)

## Введение
Реакция и разбор сработавших алертов - основная часть работы дежурного, большое количество алертов создает когнитивную нагрузку и если ложных срабатываний много,
легко пропустить действительно важное событие.
Фикс таких флапов тоже сложый процесс. Особенно в наших реалиях - много сервисов с разными паттернами трафика, разный трафик в разные дни недели.
Делая фикс для конкретного кейса легко "переобучить" алерт и впредь он будет фолсить еще больше.

Основным инструментом тестирования изменений раньше был [web ui разладок](https://razladki-wow.n.yandex-team.ru/workspace/1694/version/3)
![](https://jing.yandex-team.ru/files/ddemidov/Снимок%20экрана%202021-03-03%20в%2010.27.09.png)

Ссылки на нужный workspace разладок хранились в программе алерта в качестве комментария. Можно было поменять программу и сразу увидеть ее работу на исторических данных.

**Основные недостатки такого подхода**
1. Актуальная программа алерта могла быть неконсинстентна версии в Разладках (например, поменяли "по-быстрому" какой-то коэффициент)
2. Расчет — длительный процесс и считать большое количество разных рядов все равно приходилось в Нирване
3. Разладки считают только "плохие точки" во временных рядах из Соломона, дальше эти точки отправляются в Джаглер, где есть дополнительная логика - флаподавы, окно оценки, задержки

**Решение**
Автоматически оценивать алерты на исторических данных при их заведении и использовать только программы, прошедшие тестирование.

У этого решения 3 части
1. Автоматическое обновление программ алертов и конфигураций в Джаглере
2. Процесс разметки исторических данных
3. Пайплайн расчета в Нирване

## Автоматическое обновление программ алертов и конфигураций в Джаглере
В Аркадии хранятся программы алертов и конфигурации агрегатов. Менять их нужно именно в этом месте:
1. [Программы алертов](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/zfp/solomon_alerts)
2. [Конфигурации Juggler](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/zfp/juggler_aggregates)

На прекоммитных проверках CI запускает пайплайн расчета в Нирване с помощью [Sandbox-таски](https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/antiadblock/zfp/run_calculation/__init__.py).  
Пайплайн производит расчеты (подробнее в пункте про него), успешное завершение пайплайна является обязательной прекоммитной проверкой.

[Sandbox-таска автозаведения](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/tasks/monitoring_update/__main__.py) обновляет программы и агрегаты из транка.  
Таска работает каждый день, [шедулер](https://sandbox.yandex-team.ru/scheduler/698683/view)

## Процесс разметки исторических данных
Для того, чтобы тестировать алерты автоматически нужен тест-сет, чтобы автоматика могла понять, какое срабатывание является ожидаемым, а какое нет.
Такой тест-сет мы собираем руками дежурных и храним в очереди [ANTIADBALERTS](https://st.yandex-team.ru/ANTIADBALERTS).

### Как собираем
К каждому сообщению мониторинга прикреплена ссылка, по которой можно создать тикет в нужном формате.

**Обязательные поля в тикете**
1. Заголовок должен быть в формате juggler_host:juggler_service (автоматически подставляется при создании тикета по ссылке)
2. Тэг - тип флапа. Должен быть false_positive или false_negative (автоматически подставляется при создании тикета по ссылке)
3. Начало инцидента — момент срабатывания мониторинга. Нужно взять из сообщения мониторинга.
![](https://jing.yandex-team.ru/files/ddemidov/Снимок%20экрана%202021-03-03%20в%2012.20.53.png)

Когда тикет заведен, его нужно проверить и выбрать резолюцию - "Решен" (если тикет заведен на действительно ложное срабатывание) или "Некорректный" (если тикет заведен по ошибке).
Решенные тикеты используются в тест-сете, некорректные — игнорируются.
![](https://jing.yandex-team.ru/files/ddemidov/Снимок%20экрана%202021-03-03%20в%2012.44.13.png)

Этот дополнительный шаг проверки нужен, потому что мы исходим из того, что лучше завести неправильный тикет и потом его закрыть, чем не завести тикет на реальную проблему.

Данные по срабатываниям складываются в [отчет на Стате](https://stat.yandex-team.ru/AntiAdblock/monitoring_stats) [Sandbox-таской](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/tasks/monitoring_stats).  
По ним также строится [график](https://datalens.yandex-team.ru/preview/uzinyk1gt58ap-monitoring-stats).  

#### Упрощение заведения тикетов
Для упрощения заведения тикетов можно использовать Sandbox-таску **AAB_ZFP_CHECK_ALERTS**. При создании таски нужно выбрать требуемые juggler_hosts и service_ids, а также ввести дату начала и дату окончания проверки. 
Таска грепает за указанный период из джаглера историю срабатывания проверок и достает заведенные тикеты из стартрека и строит отчет по алертам, для которых нет заведенных тикетов. 
В отчете есть ссылка на график (ссылка на график в Solomon достается из поля description сработавшей проверки) и ссылка для быстрого заведения тикета (все необходимые поля заполнены, нужно только нажать кнопку Создать). 
Отчет в UI можно найти на странице **ALERTS REPORT**. [Примет таски](https://sandbox.yandex-team.ru/task/1204904545/report).

### Разметка ложно не сработавших алертов
Для разметки ложно не сработавших алертов (false_negative) есть [форма заведения тикета в Стартреке](https://st.yandex-team.ru/createTicket?queue=ANTIADBALERTS&_form=93168)  
Нужно выбрать juggler_host и service_id, выбрать дату инцидента. К сожалению, в Формах нет поля с типом дата/время, поэтому после заведения тикета нужно указать время в поле **Начало инцидента**

## Пайплайн расчета в Нирване
[Граф в Nirvana](https://nirvana.yandex-team.ru/process/2588d7e9-c5cc-4a52-a054-7746c94bb281/graph)  
**Параметры пайплайна**
- alert_ids - id алертов, для которых производится расчет
- juggler_hosts - хосты джаглер агрегатов, для которых производится оценка
- period_from - timestamp начала исторических данных
- period_to - timestamp конца исторических данных
- debug-timeout - время (в минутах) в течение которого будут доступны контейнеры с операциями в случае неуспешного завершения

**Общий алгоритм пайплайна**
1. Загрузить необходимые данные из Соломона
2. Рассчитать по данным "плохие точки"
3. Оценить срабатывания агрегатов
4. Сравнить смоделированные срабатывания с тест-сетом
5. Отправить письмо с результатом работы

Теперь подробнее про каждый пункт

### Загрузка данных из Solomon и расчет плохих точек
[Операция в Нирване](https://nirvana.yandex-team.ru/operation/345dc34e-4199-41b6-82db-a39c8b5708b9)

#### Загрузка
[Скрипт загрузки](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/zfp/nirvana_data_loader.py)  
Загрузка данных происходит в 2 этапа:
1. Вычисление необходимых селекторов по программе алерта.
   По сути, это обертка над java-программой [Solomon calculator](https://a.yandex-team.ru/arc/trunk/arcadia/razladki/razladki_wow/razladki_wow/engines/solomon/calculator)
2. Собственно, загрузка данных по селекторам.
   Тут мы используем разладочный [Solomon processor](https://a.yandex-team.ru/arc/trunk/arcadia/razladki/razladki_wow/razladki_wow/engines/solomon/processor.py),
   пропатчив некоторые классы

Данные складываются в tsv в output директорию, путь к которой затем передается, как инпут в расчет плохих точек

#### Расчет плохих точек
[Скрипт расчета](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/zfp/nirvana_executor.py)  
Это просто питоновская обертка над все тем же [Solomon calculator](https://a.yandex-team.ru/arc/trunk/arcadia/razladki/razladki_wow/razladki_wow/engines/solomon/calculator)  
Для ускорения расчетов параллельно запускаются 16 процессов  
Результат сохраняется в tsv файле в формате `timestamp\t(ALARM|OK)`

### Оценка Juggler-агрегатов
Для оценки агрегатов был реализован juggler-эмулятор, [код](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/zfp/juggler_emulator/emulator.py)  
Алгоритм следующий:
1. Загрузка данных, расчитанных Solomon калькультором
1. Применение конфига флаподава, [код](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/zfp/juggler_emulator/emulator.py?rev=r9088304#L95)
1. Применение логики hold_crit, [код](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/zfp/juggler_emulator/emulator.py?rev=r9088304#L148)
1. Загрузка данных алертов, от которых зависит текущий агрегат (если зависит от других агрегатов, то они должны быть уже расчитаны)
1. Расчет агрегата (реализованы logic_or, logic_and, more_than_limit_is_crit, timed_more_than_limit_is_problem)
1. Учет значений алертов, от которых зависит текущий агрегат (unreach_checks)
1. Сохранение результата в tsv файле в формате `timestamp\t(CRIT|OK)`

### Расчет диффа и релиз алертов
[Код](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/zfp/diff_calculator/__main__.py)  
Алгоритм построения диффа (для каждого агрегата):
1. Загружаем историю срабатываний из juggler
1. Загружаем размеченные ложные срабатывания/несрабатывания из Стартрека (ANTIADBALERTS)
1. Сравниваем расчитанные значения и исторические, строим дифф с учетом разметки срабатываний
1. Загружаем дифф как SB-ресурс
1. Отправляем письмо с результатами расчетом диффа
Пример диффа  
![](https://jing.yandex-team.ru/files/dridgerve/Снимок%20экрана%202022-03-09%20в%2012.27.31.png)
