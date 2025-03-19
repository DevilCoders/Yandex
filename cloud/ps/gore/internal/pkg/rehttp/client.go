// rehttp is a package with wrappers for making common http requests with retries
package rehttp

import (
	"time"

	"github.com/cenkalti/backoff/v4"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/auth"
)

// Client is a stucture with configuration of http request with retries
// default Timeout is 10 second
// default Backoff is ExponentialBackOff with default values
type Client struct {
	ServiceName  string
	Token        string
	AuthType     auth.AuthType
	ExpectedCode []int
	Timeout      time.Duration
	BackOff      backoff.BackOff
}
