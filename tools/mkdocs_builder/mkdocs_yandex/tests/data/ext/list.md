# Lists



## DL

{% list dl %}

- Вертикаль

    Совокупность [графов](#graph) с общим набором [источников](#instance), которые выкладываются в одном бандле.

- Граф

    Конфигурация, связывающая источники в единую систему для ответа на определенные виды запросов.

    Содержит:

    - Описания источников.
    - Связи между источниками.
    - Прочие параметры, влияющие на обработку запроса к AppHost.

- Инстанс (бекенд)

    Экземпляр приложения источника.

- Контекст

    Блок памяти, содержащий совокупность данных, генерируемых и потребляемых источниками в процессе
    ответа на запрос.

    AppHost получает начальный контекст вместе с запросом. По мере прохождения [графа](#graph) AppHost добавляет
    в контекст ответы источников. Конечное состояние контекста — это ответ на запрос.

    Структурно контекст состоит из множества полей, которые содержат бинарные или JSON данные.
    Каждое поле имеет определенный тип, который используется источниками при чтении/записи данных.

{% endlist %}



## Tabs

{% list tabs %}

- Python

    _python_
    **import os**

    {% list details %}

    - Cut

        Inside

    {% endlist %}

- Java

    _java_
    **class HelloWorld {**

    {% list radio %}

    - Button 1

        Inside

    - Button 2

        Inside

    {% endlist %}

    And something else.

- Just Tab

    - One 1
    - Two 1
    - Three 1

    {% list dropdown %}

    - Item 1

        Inside

    - Item 2

        Inside

    {% endlist %}

{% endlist %}



## Details

{% list details %}

- Python

    _python_
    **import os**

- Java

    _java_
    **class HelloWorld {**

    And something else.

- Just Tab

    - One 1
    - Two 1
    - Three 1

        {% note %}

        Note it.

        {% endnote %}

{% endlist %}



## Radio

{% list radio %}

- Python

    _python_
    **import os**

- Java

    _java_
    **class HelloWorld {**

    And something else.

- Just Tab

    - One 1
    - Two 1
    - Three 1

{% endlist %}



## Dropdown

{% list dropdown %}

- Python

    _python_
    **import os**

- Java

    _java_
    **class HelloWorld {**

    And something else.

- Just Tab

    - One 1
    - Two 1
    - Three 1

{% endlist %}
