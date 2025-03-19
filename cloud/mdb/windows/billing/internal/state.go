package internal

import (
	"encoding/json"
	"io/ioutil"
	"os"
	"time"
)

type State struct {
	LastTS time.Time `json:"last_ts"`
	Alive  bool      `json:"alive"`
}

const StateTTL = 300 * time.Second

func (app *App) getState() (*State, error) {
	data, err := ioutil.ReadFile(app.Cfg.StatePath())
	if err != nil {
		return nil, err
	}
	state := new(State)
	err = json.Unmarshal(data, state)
	if err != nil {
		return nil, err
	}
	return state, nil
}

func (app *App) GetState() *State {
	state, err := app.getState()
	if err != nil {
		if !os.IsNotExist(err) {
			app.L().Errorf("failed to load state: %s", err)
		}
		return new(State)
	}
	if time.Since(state.LastTS) > StateTTL {
		app.L().Errorf("prev state is too old: %s", state.LastTS)
		return new(State)
	}
	return state
}

func (app *App) SetState(s *State) error {
	data, err := json.Marshal(s)
	if err != nil {
		return err
	}
	return ioutil.WriteFile(app.Cfg.StatePath(), data, 0644)
}
