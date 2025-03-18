package httplaas

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/yandex/laas"
)

func TestGolden(t *testing.T) {
	c, err := NewClient(AsService("arcadia"))
	assert.NoError(t, err)

	ctx := context.Background()
	resp, err := c.DetectRegion(ctx, laas.Params{
		YandexGID: 213,
		URLPrefix: "http://a.yandex-team.ru/arcadia/library/go/yandex/laas",
	})

	assert.NoError(t, err)
	assert.NotNil(t, resp)
}
