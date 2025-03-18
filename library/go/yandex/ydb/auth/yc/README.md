# yc

helpers to connect to YDB inside yandex-cloud

## Usage <a name="Usage"></a>

```go
import (
  "fmt"
  "sync/mutex"
  "time"

  "go.uber.org/zap"

  "github.com/ydb-platform/ydb-go-sdk/v3"
  "github.com/ydb-platform/ydb-go-sdk/v3/trace"

  yc "a.yandex-team.ru/library/go/yandex/ydb/auth/yc"
)

func main() {
  db, err := ydb.New(
    ctx,
    connectParams,
    yc.WithInternalCA(),
    //yc.WithMetadataCredentials(ctx), // auth inside cloud (virual machine or yandex function)
    yc.WithIAM(
      makeCreateToken(yc.DefaultEndpoint, x509),
      yc.WithIamServiceFile("~/.ydb/sa.json"),
    ), // auth from service account key file
  )
```

### Common CreateTokenFunc

`yc.CreateTokenFunc` implementation exclude from package `yc` for exclude dependency to
`a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/iam/v1` and must implements on client side

```go
import (
  "context"
  "time"

  "google.golang.org/grpc"

  v1 "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/iam/v1"

  yc "a.yandex-team.ru/library/go/yandex/ydb/auth/yc"
)

// makeCreateTokenFunc returns yc.CreateTokenFunc
// If some args not need - drop it
func makeCreateTokenFunc(
  endpoint string,
  opts ...grpc.DialOption,
) yc.CreateTokenFunc {
  return func(ctx context.Context, jwt string) (
    token string, expires time.Time, err error,
  ) {
    conn, err := grpc.DialContext(ctx, endpoint, opts...)
    if err != nil {
      return
    }
    defer conn.Close()

    client := v1.NewIamTokenServiceClient(conn)
    res, err := client.Create(ctx, &v1.CreateIamTokenRequest{
      Identity: &v1.CreateIamTokenRequest_Jwt{
        Jwt: jwt,
      },
    })
    if err == nil {
      token = res.IamToken
      expires = time.Unix(
        res.ExpiresAt.Seconds,
        int64(res.ExpiresAt.Nanos),
      )
    }
    return
  }
}
```
