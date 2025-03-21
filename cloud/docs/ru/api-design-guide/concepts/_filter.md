# Фильтрация результатов

Метод `List` позволяет фильтровать результат по
определенным признакам. Например, можно получить список только тех ресурсов, у которых значение поля `name` равно
<q>test</q>.

{% note info %}

На данный момент фильтровать результаты можно только по полю `name`.

{% endnote %}

Параметры фильтрации передаются в поле `filter` и указываются в формате:

```<условие1>AND<условие2>AND<условиеN>```

Обратите внимание, регистр логического оператора не учитывается (поддерживаются оба вариана AND и and).

Каждое условие содержит:
1. Имя поля, по которому нужно осуществить фильтрацию. На данный момент поддерживается только поле `name`.
2. Операторы `=` или `!=` для одиночных значений, `IN` или `NOT IN` для списков значений.
3. Значение длиной от 3 до 63 символов, совпадающее с регулярным выражением `^[a-z][-a-z0-9]{1,61}[a-z0-9]$`.


Пример:
>```protobuf
> // Метод для получения списка дисков в указанном каталоге.
> rpc List (ListDisksRequest) returns (ListDisksResponse) {
>    option (google.api.http) = { get: "/compute/v1/disks" };
>  }
> message ListDisksRequest {
>   string folder_id = 1;
>   int64 page_size = 2;
>   string page_token = 3;
>   // Выражение для фильтрации.
>   string filter = 4;
> }
>```

В качестве примера отправим REST запрос на получение списка дисков, у которых имя равно <q>test</q>:
>??? Как в REST?
>```
>GET https://compute.{{ api-host }}/compute/v1/disks?folderId=a3s17h9sbq5asdgss12&name=test%20and%20name=dev
>```
