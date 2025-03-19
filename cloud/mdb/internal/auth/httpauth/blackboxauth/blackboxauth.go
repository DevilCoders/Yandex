package blackboxauth

import (
	"fmt"

	"a.yandex-team.ru/cloud/mdb/internal/auth/blackbox"
)

// Cache is responsible for caching blackbox replies
type Cache interface {
	Get(key string) (blackbox.UserInfo, error)
	Put(key string, value blackbox.UserInfo)
}

// Cache miss error
var (
	ErrNotInCache = fmt.Errorf("value not in cache")
)

// LoginListChecker is responsible for checking logins against white list
type LoginListChecker interface {
	Check(uinfo blackbox.UserInfo) error
}

// OAuthScopeChecker is responsible for checking token scope against white list
type OAuthScopeChecker interface {
	CheckScope(uinfo blackbox.UserInfo) error
}
