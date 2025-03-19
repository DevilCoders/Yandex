package remote_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/supervisor/remote"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestRemote(t *testing.T) {
	lg, _ := zap.New(zap.KVConfig(log.DebugLevel))
	require.NotNil(t, lg)
	require.NotNil(t, remote.New("localhost", "user", "password", lg))
}
