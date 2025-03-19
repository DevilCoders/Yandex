#!/bin/bash

if [[ "$1" =~ -(-help|h) ]]; then
    echo '
## ansible-juggler-generator.sh
Авторизация через oauth токет в переменной JUGGLER_OAUTH_TOKEN
Структура безжалостно скопипастена у ребят
из https://github.yandex-team.ru/cs-admin/ansible-juggler-configs
и немножко обработан а под нас.

Рекомендуетя ansible > 2.0.0, но < 2.3.0 + ansible_juggler2:

    sudo pip install --index-url https://pypi.yandex-team.ru/simple/  ansible-juggler2 -U

    # применение конкретного конфига
    ./ansible-juggler-generator.sh kinopoisk kp-backend [group [group]...]

    # применение всех конфигов проекта
    ./ansible-juggler-generator.sh kinopoisk

Для работы с `ansible-juggler-generator.sh`
нужно создать каталог `./projects/${project}/` и положить конфиги туда.
Примерный шаблон конфига лежит в `./templates`
Для работы так же нужен файл с переменными
`./projects/${project}/vars` с минимум двумя переменными:

    project: "kinopoisk"
    resps: ["robot-cult", "dmokhatkin", "sergeyv", "chrono", "paulus"]

В файле `./group_vars/all` лежат все common-переменные
'
exit 0
fi

if [[ "$1" =~ (-(v|-verbose)|-(d|-debug)) ]]; then
    shift
    VERBOSE=" -vvvv "
fi

play(){
  local host=${1%.yml}
  local conf="$CONFIGS_PATH/projects/${PROJECT}/${host}.yml"
  echo -e ">>>>> \e[1;93mPLAYING $conf\e[0m"
  ansible-playbook $VERBOSE -i $CONFIGS_PATH/inventory.cfg  ${conf} --extra-vars="project=$PROJECT" $DEBUG
}

CONFIGS_PATH=$(dirname "$(realpath $0)")
PROJECT="$1"
if [[ -z "$PROJECT" ]]; then
  echo "Give me PROJECT plz!"
  echo "$0 PROJECT [ HOST [ HOST [...] ] ]"
  exit 1
fi

hosts=( "${@:2}" ) # получаем аргументы с третьего по последний в виде массива
if ((${#hosts[@]} == 0)); then # если передан только проект, то обновляем все хосты
    hosts=( $(ls -1 $CONFIGS_PATH/projects/${PROJECT}/|grep .yml$) )
fi

# link tasks to PROJECT path
ln -sf $CONFIGS_PATH/tasks $CONFIGS_PATH/projects/${PROJECT}/
for name in "${hosts[@]}"; do
    play $name
done
# remove link tasks to PROJECT path after playing
rm -f $CONFIGS_PATH/projects/${PROJECT}/tasks
