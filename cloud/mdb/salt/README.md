salt
====

Конфигурации и рецепты salt-a

Клонирование производить в /srv.



*HowTo:*
Генерация списка ресурсов (valid_resources.sls) переехала в cloud/mdb/tools/vr_gen

Теперь надо
1) Перейти в cloud/mdb/tools/vr_gen
2) запустить команды, ничего не исправляя в Makefile:
```
ya make
make generate_valid_resources
```

