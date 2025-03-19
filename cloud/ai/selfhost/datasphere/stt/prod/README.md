Порядок обновления модели:

1. Обновить спеку ноды, прописав актуальную версию образа
2. Создать ноду: `./commands/create_node node-spec.json`
3. Обновить node_id в `nightly` спеках алиасов
4. Выложить модель в `nightly` ветку: `./commands/update_alias update-nightly.json`
5. Оценить качество модели с помощью [графа оценки](https://nirvana.yandex-team.ru/flow/b59a92ff-7068-4e8e-b441-02d613a99620/f9f7a0c9-80d5-4730-b9c7-c4b50a923ebc/graph), сравнить с ожидаемыми значениями
6. Прогнать `load_test`
7. Если всё ОК, то обновляем node_id в `rc` спеках алиасов, после чего обновляем `rc`: `./commands/update_alias update-rc.json`
8. Коммитим все изменения :)
