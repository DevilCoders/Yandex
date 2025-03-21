Используйте валидацию, если нужно проверить, что введенный ответ соответствует заданному формату. Например, можно проверить, что ответ содержит текст на русском языке или что в ответе нет запрещенных символов. Если ответ не соответствует заданному формату, появится предупреждение, что поле заполнено неверно.

Выберите один из методов валидации:

* **Без валидации** — ответ может содержать любые символы. При выборе этого метода валидации проверка введенных данных не производится.

{% if audience == "internal" %}
* **Внешний валидатор** — выполнять валидацию ответа с помощью внешнего сервиса. При выборе этого метода в настройках формы укажите [адрес сервера валидации](../../forms/validation.md).
{% endif %}
* **Валидация дробных чисел** — ответ должен содержать целое число или десятичную дробь. 

* **Валидация ИНН** — ответ должен содержать корректный ИНН: 10 или 12 цифр, соответствующих контрольным проверкам. 

* **Валидация кириллических символов** — ответ должен содержать буквы русского алфавита, цифры, пробелы и знаки `. , ; ( )`. 

* **Валидация через регулярные выражения** — вы можете составить свое [регулярное выражение](https://ru.wikipedia.org/wiki/Регулярные_выражения) для проверки ответа. Например: 
    * Регулярное выражение, которое разрешает вводить только буквы латинского алфавита, цифры и пробелы: 
        ```
        ^[A-Za-z0-9\s]+$
        ```
    * Регулярное выражение, которое разрешает вводить любые символы, кроме цифр и некоторых спецсимволов: 
        ```
        ^[^0-9@#$%^&*]+$
        ```