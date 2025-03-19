package disklock

import (
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/mdb-disklock/internal/keys"
)

type Config struct {
	Keys             []keys.Config            `json:"keys" yaml:"keys"`
	Disks            []Disk                   `json:"disks" yaml:"disks"`
	MountWaitTimeout encodingutil.Duration    `json:"mount_wait_timeout" yaml:"mount_wait_timeout"`
	AWSTransport     httputil.TransportConfig `json:"aws_transport" yaml:"aws_transport"`
	KeyLength        int                      `json:"key_length" yaml:"key_length"`
}

type Disk struct {
	ID    string `json:"id" yaml:"id"`
	Name  string `json:"name" yaml:"name"`
	Key   string `json:"key" yaml:"key"`
	Mount string `json:"mount" yaml:"mount"`
}

func (d *Disk) Mapper() string {
	return fmt.Sprintf("/dev/mapper/%s", d.Name)
}

func DefaultConfig() Config {
	return Config{
		MountWaitTimeout: encodingutil.FromDuration(10 * time.Second),
		AWSTransport:     httputil.DefaultTransportConfig(),
		KeyLength:        2048,
	}
}
