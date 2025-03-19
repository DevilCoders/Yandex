## Vowpal Wabbit Filters

Модуль, применяющий vw модели к паре (запрос, ответ) для определения принадлежности к определенным тематикам.


### Как обучить модель:

`vw -d medicine_train.txt -f medicine.vw --readable_model readable_medicine.vw --ngram 2 --l1 1e-8 --l2 1e-7`
 
 - `--ngram 2` — используем биграммы при обучении
 - `--l1`, `--l2` — l1, l2 регуляризацция
 - Все параметры: https://github.com/VowpalWabbit/vowpal_wabbit/wiki/Command-Line-Arguments
 - Бинарь `vw` можно, например, сбилдить из исходников: https://github.com/VowpalWabbit/vowpal_wabbit/wiki/Building
 
Больше в [SNIPPETS-8094](https://st.yandex-team.ru/SNIPPETS-8094).


### Как сконвертить модель для использования в модуле:

1. `./convert_model_tool --readable-model readable_medicine.vw --converted-model fs_medicine.vw`
2. `tar -zcf resource.tgz fs_medicine.vw`
3. `ya upload resource.tgz --ttl inf`
4. Добавить в [ya.make](https://a.yandex-team.ru/arc/trunk/arcadia/search/web/rearrs_upper/rearrange.dynamic/facts/vowpal_wabbit_filters/filters_config.json): `FROM_SANDBOX(<resource_id> OUT fs_medicine.vw)`
