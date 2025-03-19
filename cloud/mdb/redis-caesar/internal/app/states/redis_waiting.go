package states

type StateRedisWaiting struct {
}

func NewStateRedisWaiting() *StateLost {
	return &StateLost{}
}

func (s *StateRedisWaiting) Run() (AppState, error) {
	return nil, nil
}

func (s *StateRedisWaiting) Name() string {
	return "StateRedisWaiting"
}
