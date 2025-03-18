# metrics

metrics package helps to create ydb-go-sdk traces with monitoring over solomon

## Usage
```go
import (
  "github.com/ydb-platform/ydb-go-sdk/v3"
  "github.com/ydb-platform/ydb-go-sdk/v3/trace"
  "github.com/ydb-platform/ydb-go-sdk-metrics"

  "a.yandex-team.ru/library/go/core/metrics/solomon"
  ydbMetrics "a.yandex-team.ru/library/go/yandex/ydb/metrics"
)

...
  // init solomon registry
  registry := solomon.NewRegistry(solomon.NewRegistryOpts())

  db, err := ydb.New(
    ctx,
    ydb.MustConnectionString(connection),
    ydbMetrics.WithTraces(
      registry,
      ydbMetrics.WithDetails(trace.Details),
    ),
  )

```
