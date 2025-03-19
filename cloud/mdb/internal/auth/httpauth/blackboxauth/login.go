package blackboxauth

import (
	"sort"

	"a.yandex-team.ru/cloud/mdb/internal/auth/blackbox"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
)

// interface satisfaction check
var _ LoginListChecker = &LoginSortedListChecker{}

// LoginSortedListChecker is a blackboxauth addon for checking token scopes
type LoginSortedListChecker struct {
	whiteList []string
}

// Check method validates that all required scopes are in userinfo
func (c *LoginSortedListChecker) Check(uinfo blackbox.UserInfo) error {
	index := sort.SearchStrings(c.whiteList, uinfo.Login)
	if index >= len(c.whiteList) || c.whiteList[index] != uinfo.Login {
		return httpauth.ErrAuthNoRights
	}
	return nil
}

// NewLoginSortedListChecker is a LoginSortedListChecker constructor helper
func NewLoginSortedListChecker(whiteList []string) *LoginSortedListChecker {
	sorted := whiteList[:]
	sort.Strings(sorted)
	return &LoginSortedListChecker{
		whiteList: sorted,
	}
}
