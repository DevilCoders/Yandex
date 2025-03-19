package internal

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/yarn"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func TestDefaultSelectYarnApplication(t *testing.T) {
	apps := []yarn.Application{
		{StartedTime: 3, State: yarn.StateRunning},
		{StartedTime: 1, State: yarn.StateFinished},
		{StartedTime: 2, State: yarn.StateSubmitted},
	}

	jobRunner := MapreduceJobRunner{logger: &nop.Logger{}}
	app, err := DefaultSelectYarnApplication(&jobRunner, apps)
	require.NoError(t, err)
	require.Equal(t, yarn.StateFinished, app.State)
	require.Equal(t, 1, app.StartedTime)
}
