package juggler_test

import (
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/juggler"
)

func TestRawEvent_String(t *testing.T) {
	require.Equal(t, "CRIT on mdb100.y.net:unispace at 2009-01-10T23:00:00Z: 102% used on /data", juggler.RawEvent{
		Service:      "unispace",
		Host:         "mdb100.y.net",
		ReceivedTime: time.Date(2009, time.January, 10, 23, 0, 0, 0, time.UTC),
		Status:       "CRIT",
		Description:  "102% used on /data",
	}.String())
}
