package blackboxauth

import (
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/auth/blackbox"
)

func Test_CCache(t *testing.T) {
	cache := NewCCache(600*time.Hour, 100)
	info := blackbox.UserInfo{
		DisplayName: blackbox.DisplayName{Name: ""},
		Login:       "test",
	}
	_, err := cache.Get("test")
	require.Error(t, err)
	cache.Put("test", info)
	cachedInfo, err := cache.Get("test")
	require.NoError(t, err)
	require.Equal(t, info, cachedInfo)
}
