//go:build !cgo
// +build !cgo

package geobase

type Option func()

func New(opts ...Option) (Geobase, error) {
	return nil, ErrNotSupported
}

func WithRefresh(b bool) Option {
	return nil
}

func WithLockMemory(b bool) Option {
	return nil
}

func WithPreload(b bool) Option {
	return nil
}

func WithTzData(b bool) Option {
	return nil
}

func WithID9KEnabled(b bool) Option {
	return nil
}

func WithDatafilePath(path string) Option {
	return nil
}

func WithTimezonesPath(path string) Option {
	return nil
}
