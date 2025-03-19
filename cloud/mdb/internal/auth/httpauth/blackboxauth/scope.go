package blackboxauth

import (
	"sort"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/auth/blackbox"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
)

// interface satisfaction check
var _ OAuthScopeChecker = &OAuthScopeListChecker{}

// OAuthScopeListChecker is a blackboxauth addon for checking token scopes using simple list of scopes
type OAuthScopeListChecker struct {
	scopes []string
}

// CheckScope method validates that all required scopes are in userinfo
func (c *OAuthScopeListChecker) CheckScope(uinfo blackbox.UserInfo) error {
	if uinfo.Scope == "" {
		return nil
	}

	uscopes := strings.Fields(uinfo.Scope)
	sort.Strings(uscopes)

	for _, req := range c.scopes {
		index := sort.SearchStrings(uscopes, req)
		if index >= len(uscopes) || uscopes[index] != req {
			return httpauth.ErrAuthNoRights
		}
	}
	return nil
}
