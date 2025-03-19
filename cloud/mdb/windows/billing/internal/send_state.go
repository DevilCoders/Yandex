package internal

import (
	"encoding/json"
	"io/ioutil"
	"os"
	"time"
)

type SendState struct {
	SentTS int64 `json:"sent_ts"`
}

func (app *App) getSendState() (*SendState, error) {
	state := new(SendState)
	data, err := ioutil.ReadFile(app.Cfg.StateSendPath())
	if err != nil {
		return nil, err
	}
	err = json.Unmarshal(data, state)
	if err != nil {
		return nil, err
	}
	return state, nil
}

func (app *App) GetSendState() *SendState {
	state, err := app.getSendState()
	if err != nil {
		state = new(SendState)
		if os.IsNotExist(err) {
			return state
		}
		app.L().Errorf("failed to read send state file: %v", err)
		// do not overbill
		state.SentTS = time.Now().Unix()
		err := app.SetSendState(state)
		if err != nil {
			app.L().Errorf("failed to write default send state file: %v", err)
		}
		return state
	}
	return state
}

func (app *App) SetSendState(s *SendState) error {
	data, err := json.Marshal(s)
	if err != nil {
		return err
	}
	return ioutil.WriteFile(app.Cfg.StateSendPath(), data, 0644)
}
