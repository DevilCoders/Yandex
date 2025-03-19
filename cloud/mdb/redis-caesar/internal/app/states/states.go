package states

//go:generate $GOPATH/bin/mockgen -source ./states.go -destination ./mock/states.go -package mock

// AppState is an interface that should be implemented by all states that application can enter.
type AppState interface {
	// Run is a main function that should be called when app should enter specific state.
	Run() (nextState AppState, err error)
	// Name is a function that returns name of state. Useful for debugging purposes.
	Name() string
}
