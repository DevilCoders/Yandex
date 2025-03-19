package models

import (
	"io/ioutil"
	"path/filepath"
	"strings"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/config"
	"a.yandex-team.ru/library/go/test/yatest"
)

func TestConfigFields(t *testing.T) {
	configPath := yatest.SourcePath("cloud/mdb/mdb-maintenance/configs")
	files, err := ioutil.ReadDir(configPath)
	require.NoError(t, err)

	for _, f := range files {
		if f.IsDir() {
			continue
		}
		configID := strings.TrimSuffix(f.Name(), filepath.Ext(f.Name()))
		t.Run(configID, func(t *testing.T) {
			var cfg = DefaultTaskConfig()
			err := config.LoadFromAbsolutePath(filepath.Join(configPath, f.Name()), &cfg)
			require.NoError(t, err)
			require.NotZero(t, cfg.MaxDays, "max days should not be zero")
			require.Less(t, time.Duration(cfg.MaxDays)*time.Hour*24, MaxDelayDaysUpperBound)
			require.Less(t, cfg.MinDays, cfg.MaxDays)
			require.NotEmpty(t, cfg.Info)
			require.False(t, cfg.ClustersSelection.Empty())
			require.NotEmpty(t, cfg.PillarChange)
			require.NotEmpty(t, cfg.Worker.OperationType)
			require.NotEmpty(t, cfg.Worker.TaskType)
			require.False(t, cfg.Worker.TaskArgs != nil && cfg.Worker.TaskArgsQuery != "", "TaskArgs and TaskArgsQuery are mutually exclusive")
		})
	}
}
