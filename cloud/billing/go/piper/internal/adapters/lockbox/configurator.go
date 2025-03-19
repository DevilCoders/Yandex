package lockbox

import (
	"context"
	"errors"
	"strings"
	"sync"
	"time"

	"github.com/cenkalti/backoff/v4"

	lbx "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/lockbox"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
)

type Configurator struct {
	client   LockboxClient
	secretID string

	secretData map[string]lbx.SecretValue
	loadErr    error
	loadOnce   sync.Once
}

type LockboxClient interface {
	GetSecret(ctx context.Context, key string) (secret map[string]lbx.SecretValue, err error)
}

func NewConfigurator(runCtx context.Context, client LockboxClient, secretID string) *Configurator {
	return &Configurator{
		client:   client,
		secretID: secretID,
	}
}

func (c *Configurator) GetConfig(ctx context.Context, namespace string) (result map[string]string, err error) {
	c.load(ctx)
	if c.loadErr != nil {
		return nil, c.loadErr
	}
	nsPrefix := namespace + "."

	result = make(map[string]string)
	for secretKey, secretValue := range c.secretData {
		if strings.HasPrefix(secretKey, nsPrefix) && secretValue.Text != "" {
			key := strings.TrimPrefix(secretKey, nsPrefix)
			result[key] = secretValue.Text
		}
	}
	return result, err
}

func (c *Configurator) GetYaml(ctx context.Context) ([]byte, error) {
	c.load(ctx)
	data := c.secretData["yaml"].Data
	if len(data) == 0 {
		data = []byte(c.secretData["yaml"].Text)
	}
	return data, c.loadErr
}

func (c *Configurator) load(ctx context.Context) {
	c.loadOnce.Do(func() {
		retryCtx := tooling.StartRetry(ctx)
		_ = c.retry(retryCtx, func() (resErr error) {
			tooling.RetryIteration(retryCtx)
			c.secretData, c.loadErr = c.client.GetSecret(retryCtx, c.secretID)
			return c.loadErr
		})
	})
}

func (c *Configurator) retry(ctx context.Context, op backoff.Operation) error {
	ebo := backoff.NewExponentialBackOff()
	ebo.InitialInterval = time.Millisecond * 20
	ebo.MaxInterval = time.Second
	ebo.MaxElapsedTime = time.Second * 5
	ebo.RandomizationFactor = 0.25

	bo := backoff.WithContext(ebo, ctx)

	checkedOp := func() error {
		return retryError(op())
	}

	return backoff.Retry(checkedOp, bo)
}

func retryError(err error) error {
	if err == nil {
		return err
	}
	if errors.Is(err, lbx.ErrNotFound) {
		return nil
	}
	return err
}
