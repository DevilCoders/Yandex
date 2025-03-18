
[Подробная инструкция](interfaces/cli.md)

<details markdown="1">
<summary>Пример использования</summary>
``` sh
$ ./yql -h # help
$ ./yql hahn <<< "select * from [home/yql/tutorial/users];" # запрос для выполнения на hahn через stdin
$ ./yql -i test.sql hahn # запрос из файла
$ ./yql # интерактивный режим
Welcome to YQL Console Client! Type help; for help.
> use hahn; select * from [home/yql/tutorial/users];
```
</details>