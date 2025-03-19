#!/bin/bash

set -e

# Google API
find ../internal/yandex/cloud/priv -type f -iname '*.proto' -print0 | xargs -0 sed -i 's/"google\/api/"cloud\/mdb\/internal\/third_party\/googleapis\/google\/api/g'
find ../internal/yandex/cloud/priv -type f -iname '*.proto' -print0 | xargs -0 sed -i 's/"google\/rpc/"cloud\/mdb\/internal\/third_party\/googleapis\/google\/rpc/g'
find ../internal/yandex/cloud/priv -type f -iname '*.proto' -print0 | xargs -0 sed -i 's/"google\/type/"cloud\/mdb\/internal\/third_party\/googleapis\/google\/type/g'

# Cloud imports
find ../internal/yandex/cloud/priv -type f -iname '*.proto' -print0 | xargs -0 sed -i 's/"yandex/"cloud\/mdb\/internal\/yandex/g'

# Go imports
# PG
find ../internal/yandex/cloud/priv/mdb/postgresql/v1/config -type f -iname '*.proto' -print0 | xargs -0 sed -i 's/option go_package = "postgresql";/option go_package = "a.yandex-team.ru\/cloud\/mdb\/internal\/yandex\/cloud\/priv\/mdb\/postgresql\/v1\/config";/g'
find ../internal/yandex/cloud/priv/mdb/postgresql/v1/console -type f -iname '*.proto' -print0 | xargs -0 sed -i 's/option go_package = "postgresql_console";/option go_package = "a.yandex-team.ru\/cloud\/mdb\/internal\/yandex\/cloud\/priv\/mdb\/postgresql\/v1\/console";/g'
find ../internal/yandex/cloud/priv/mdb/postgresql -type f -iname '*.proto' -print0 | xargs -0 sed -i 's/option go_package = "postgresql";/option go_package = "a.yandex-team.ru\/cloud\/mdb\/internal\/yandex\/cloud\/priv\/mdb\/postgresql\/v1";/g'
# CH
find ../internal/yandex/cloud/priv/mdb/clickhouse/v1/config -type f -iname '*.proto' -print0 | xargs -0 sed -i 's/option go_package = "clickhouse";/option go_package = "a.yandex-team.ru\/cloud\/mdb\/internal\/yandex\/cloud\/priv\/mdb\/clickhouse\/v1\/config";/g'
find ../internal/yandex/cloud/priv/mdb/clickhouse/v1/console -type f -iname '*.proto' -print0 | xargs -0 sed -i 's/option go_package = "clickhouse_console";/option go_package = "a.yandex-team.ru\/cloud\/mdb\/internal\/yandex\/cloud\/priv\/mdb\/clickhouse\/v1\/console";/g'
find ../internal/yandex/cloud/priv/mdb/clickhouse -type f -iname '*.proto' -print0 | xargs -0 sed -i 's/option go_package = "clickhouse";/option go_package = "a.yandex-team.ru\/cloud\/mdb\/internal\/yandex\/cloud\/priv\/mdb\/clickhouse\/v1";/g'
# Operation
find ../internal/yandex/cloud/priv/operation -type f -iname '*.proto' -print0 | xargs -0 sed -i 's/option go_package = "operation";/option go_package = "a.yandex-team.ru\/cloud\/mdb\/internal\/yandex\/cloud\/priv\/operation";/g'
# Access service
find ../internal/yandex/cloud/priv/servicecontrol -type f -iname '*.proto' -print0 | xargs -0 sed -i 's/option go_package = "servicecontrol";/option go_package = "a.yandex-team.ru\/cloud\/mdb\/internal\/yandex\/cloud\/priv\/servicecontrol\/v1";/g'
