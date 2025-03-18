package pusher

import (
	"context"

	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

const (
	HostProduction   string = "https://solomon.yandex.net"
	HostPrestable    string = "https://solomon-prestable.yandex.net"
	HostTesting      string = "https://solomon-testing.yandex.net"
	HostCloudPreprod string = "https://solomon.cloud-preprod.yandex-team.ru"
	HostCloudProd    string = "https://solomon.cloud.yandex-team.ru"

	TVMIDProduction   = 2010242
	TVMIDPrestable    = 2010240
	TVMIDTesting      = 2010238
	TVMIDCloudPreprod = 2015583
	TVMIDCloudProd    = 2015581
)

type Pusher interface {
	Push(context.Context, *solomon.Metrics) error
}
