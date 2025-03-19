package roller_test

import (
	"sync"
	"sync/atomic"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/models"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/roller"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func oneHostCluster(h string) models.Cluster {
	return models.Cluster{
		Hosts: map[string]tags.HostTags{h: {}},
	}
}

func TestRollClusters(t *testing.T) {
	t.Run("for empty host it works", func(t *testing.T) {
		err := roller.RollClusters(
			nil,
			func(models.Cluster) error {
				return nil
			},
			roller.StrictLinearAccelerator(42), &nop.Logger{})
		require.NoError(t, err)
	})

	t.Run("it call handle n-times", func(t *testing.T) {
		var called int32
		logger, _ := zap.New(zap.KVConfig(log.DebugLevel))

		err := roller.RollClusters(
			make([]models.Cluster, 42),
			func(models.Cluster) error {
				atomic.AddInt32(&called, 1)
				return nil
			},
			roller.StrictLinearAccelerator(5),
			logger)

		require.NoError(t, err)
		require.Equal(t, int32(42), called)
	})

	t.Run("stop when got error", func(t *testing.T) {
		clusters := []models.Cluster{
			oneHostCluster("a"),
			oneHostCluster("b"),
			oneHostCluster("c"),
			oneHostCluster("d"),
			oneHostCluster("e"),
			oneHostCluster("f"),
		}

		var seen []string
		var seenLock sync.Mutex
		err := roller.RollClusters(
			clusters,
			func(c models.Cluster) error {
				// it is not a cycle,
				// it is `list(c.Host)[0]`
				for f := range c.Hosts {
					seenLock.Lock()
					seen = append(seen, f)
					seenLock.Unlock()
					if f > "a" {
						return xerrors.New("Stop on values greater then a")
					}
				}
				return nil
			},
			roller.StrictLinearAccelerator(5),
			&nop.Logger{})

		require.Error(t, err)
		for _, notStarted := range []string{"d", "e", "f"} {
			require.NotContains(t, seen, notStarted)
		}
	})
	t.Run("it ignore skipped clusters", func(t *testing.T) {

		clusters := []models.Cluster{
			oneHostCluster("a"),
			oneHostCluster("b"),
			oneHostCluster("c"),
		}

		err := roller.RollClusters(
			clusters,
			func(c models.Cluster) error {
				for f := range c.Hosts {
					if f > "a" {
						return cluster.ErrRolloutSkipped
					}
				}
				return nil
			},
			roller.StrictLinearAccelerator(5),
			&nop.Logger{})

		require.NoError(t, err)
	})
}
