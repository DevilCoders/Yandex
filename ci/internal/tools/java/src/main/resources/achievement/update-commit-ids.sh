ya ydb -e ydb-ru.yandex.net:2135 -d "/ru/ci/stable/ci/" scripting yql -f select-commits.sql --format json-unicode | jq '.commitId' -r | sort | uniq > commit-ids.txt
wc -l commit-ids.txt
ya ydb -e ydb-ru.yandex.net:2135 -d "/ru/ci/stable/ci/" scripting yql -f count-total.sql