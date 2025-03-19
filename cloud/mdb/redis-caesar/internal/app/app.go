package app

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal/app/states"
	"a.yandex-team.ru/cloud/mdb/redis-caesar/pkg/config"
	"a.yandex-team.ru/library/go/core/log"
)

// stateMachine is a struct that handles the application work flow.
// It contains info about which state is active at certain moment.
type stateMachine struct {
	logger       log.Logger
	conf         *config.Global
	currentState states.AppState
}

// NewStateMachine is a function that creates a new state machine.
func NewStateMachine(logger log.Logger, conf *config.Global) *stateMachine {
	return &stateMachine{
		logger:       logger,
		conf:         conf,
		currentState: nil,
	}
}

// Start is a function that starts a state machine.
func (m *stateMachine) Start(ctx context.Context, state states.AppState) {
	m.currentState = state
	for {
		select {
		case <-ctx.Done():
			return
		default:
			m.logger.Infof("caesar state: %s", state.Name())
			stateHandler := m.currentState
			if stateHandler == nil {
				panic(fmt.Sprintf("unknown state: %s", m.currentState))
			}
			nextState, err := stateHandler.Run()
			if err != nil {
				// TODO(litleleprikon): Check error here
				return
			}
			m.currentState = nextState
		}
	}
}
