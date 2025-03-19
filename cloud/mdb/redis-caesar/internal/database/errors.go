package database

import (
	"fmt"

	"a.yandex-team.ru/cloud/mdb/redis-caesar/internal"
)

type PingErrorBuilder struct {
	hosts []internal.RedisHost
}

func NewPingErrorBuilder() *PingErrorBuilder {
	return &PingErrorBuilder{
		hosts: []internal.RedisHost{},
	}
}

func (b *PingErrorBuilder) AddHost(host internal.RedisHost) {
	b.hosts = append(b.hosts, host)
}

func (b *PingErrorBuilder) Build() error {
	if len(b.hosts) > 0 {
		return PingError{
			hosts: b.hosts,
		}
	}

	return nil
}

// PingError is returned when specific host cannot be pinged.
type PingError struct {
	hosts []internal.RedisHost
}

func (e PingError) Error() string {
	return fmt.Sprintf("hosts %v cannot be pinged", e.hosts)
}

func (e PingError) Hosts() []internal.RedisHost {
	return e.hosts
}
