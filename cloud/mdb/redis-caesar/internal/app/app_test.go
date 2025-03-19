package app

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal/app/states"
	states_mock "a.yandex-team.ru/cloud/mdb/redis-caesar/internal/app/states/mock"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/pkg/config"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestStart(t *testing.T) {
	if true == false {
		t.Error("Fail")
	}
}

func Test_stateMachine_start(t *testing.T) {
	logger, _ := zap.NewDeployLogger(log.DebugLevel)

	t.Run("normal state change", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()

		firstState := states_mock.NewMockAppState(ctrl)
		secondState := states_mock.NewMockAppState(ctrl)

		m := &stateMachine{
			logger: logger,
			conf:   &config.Global{},
		}

		ctx, cancel := context.WithCancel(context.Background())

		firstState.EXPECT().Run().DoAndReturn(func() (states.AppState, error) {
			cancel()

			return secondState, nil
		})
		firstState.EXPECT().Name().Return("firstState")

		m.Start(ctx, firstState)

		assert.Equal(t, secondState, m.currentState)
	})
}

func Test_newStateMachine(t *testing.T) {
	logger, _ := zap.NewDeployLogger(log.DebugLevel)
	machine := NewStateMachine(logger, &config.Global{})
	assert.Equal(t, &stateMachine{
		logger:       logger,
		conf:         &config.Global{},
		currentState: nil,
	}, machine)
}
