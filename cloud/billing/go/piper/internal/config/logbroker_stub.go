//go:build !cgo

package config

import (
	"a.yandex-team.ru/cloud/billing/go/piper/internal/config/cfgtypes"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
)

// makeLogbrokerService is stub for tests without cgo dependencies from persqueue
func (c *Container) makeLogbrokerService(
	name string, instance int, cfg cfgtypes.LogbrokerSourceConfig, cons consumerSelector,
	handler lbtypes.Handler, offsets lbtypes.OffsetReporter,
) (logbrokerServer, error) {
	panic("CGO disabled")
}

// makeLBWriter is stub for tests without cgo dependencies from persqueue
func (c *Container) makeLBWriter(cfg cfgtypes.LogbrokerSinkConfig) (lbtypes.ShardProducer, error) {
	panic("CGO disabled")
}
