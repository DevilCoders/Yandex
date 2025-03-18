# Cuts

## Tabs

{% list tabs %}

- Cut inside tab

    {% cut %}

    Inside Cut

    {% endcut %}

- Java

    And something else.

{% endlist %}


## Details

{% cut %}

Noname cut

_python_
**import os**

{% endcut %}

{% cut "Java" %}

_java_
**class HelloWorld {**

And something else.

{% endcut %}

{% cut "По-русски" %}

- One 1
- Two 1
- Three 1

    {% note %}

    Note it inside cut.

    {% endnote %}

{% endcut %}



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
