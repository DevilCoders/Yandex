package qtool

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

func TestDateStringScan(t *testing.T) {
	want := time.Date(2022, time.April, 17, 0, 0, 0, 0, time.Local)

	v := DateString{}
	err := v.Scan("2022-04-17")

	assert.NoError(t, err)
	assert.Equal(t, want, time.Time(v))
}

func TestDateStringValue(t *testing.T) {
	want := ydb.UTF8Value("2022-04-17")
	v := DateString{}
	err := v.Scan("2022-04-17")
	assert.NoError(t, err)

	got := v.Value()

	assert.NoError(t, err)
	assert.Equal(t, want, got)
}
