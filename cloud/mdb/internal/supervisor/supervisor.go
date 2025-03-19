package supervisor

// Services provides control interface to services managed by supervisor
type Services interface {
	Start(name string) error
	Stop(name string) error
}
