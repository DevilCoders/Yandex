package swagger

import "context"

type pinger struct {
	pingers []func(context.Context) error
}

// PingRegisterer can register custom ping functions
type PingRegisterer interface {
	RegisterPing(func(context.Context) error)
}

func (p *pinger) Ping(ctx context.Context) error {
	for _, check := range p.pingers {
		err := check(ctx)
		if err != nil {
			return err
		}
	}
	return nil
}

func (p *pinger) IsReady(ctx context.Context) error {
	return p.Ping(ctx)
}

func (p *pinger) RegisterPing(f func(context.Context) error) {
	p.pingers = append(p.pingers, f)
}
