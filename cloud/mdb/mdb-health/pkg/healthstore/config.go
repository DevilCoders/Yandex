package healthstore

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
)

const (
	defaultBufSize = 10000
)

type Config struct {
	MaxWaitTime encodingutil.Duration `json:"max_wait_time" yaml:"max_wait_time"`
	MinBufSize  int                   `json:"min_buf_size" yaml:"min_buf_size"`
	MaxBuffSize int                   `json:"max_buff_size" yaml:"max_buff_size"`
	MaxCycles   int                   `json:"max_cycles" yaml:"max_cycles"`
}

func DefaultConfig() Config {
	return Config{
		MinBufSize:  defaultBufSize,
		MaxBuffSize: defaultBufSize * 10,
		MaxWaitTime: encodingutil.FromDuration(time.Second),
	}
}
