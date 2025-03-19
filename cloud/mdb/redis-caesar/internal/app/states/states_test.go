package states

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/mdb/redis-caesar/pkg/config"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func Test_newStateFirstRun(t *testing.T) {
	conf := &config.Global{}
	logger, _ := zap.NewDeployLogger(log.DebugLevel)
	assert.Equal(t, &StateFirstRun{conf: conf, logger: logger}, NewStateFirstRun(conf, logger))
}
