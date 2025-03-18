package recovery

import (
	"fmt"
	"net/http"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestWithLogger(t *testing.T) {
	var mw middleware

	input, err := zap.NewQloudLogger(log.DebugLevel)
	require.NoError(t, err)

	opt := WithLogger(input)
	opt(&mw)

	assert.Same(t, input, mw.l)
}

func TestWithCallBack(t *testing.T) {
	var mw middleware

	callback := func(_ http.ResponseWriter, _ *http.Request, _ error) {}

	opt := WithCallBack(callback)
	opt(&mw)

	assert.Equal(t, fmt.Sprintf("%p", callback), fmt.Sprintf("%p", mw.panicCallback))
}
