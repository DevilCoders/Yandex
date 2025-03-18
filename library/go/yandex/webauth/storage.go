package webauth

import (
	"sync"
	"time"

	"github.com/jonboulle/clockwork"
)

type IDMRole struct {
	Granted   bool
	CheckedAt time.Time
}

type LoginInfo struct {
	Login     string
	CheckedAt time.Time
}

type UserRoles map[string]*IDMRole

type AuthStorage struct {
	LoginsByToken     map[string]*LoginInfo
	LoginsBySessionID map[string]*LoginInfo
	RolesByLogin      map[string]*UserRoles
	TTL               time.Duration
	clock             clockwork.Clock
	lock              sync.RWMutex
}

// getUserRoles should only be called under lock.
func (s *AuthStorage) getUserRoles(credentials string, asToken bool) (*LoginInfo, *UserRoles, bool) {
	var login *LoginInfo
	var ok bool

	if asToken {
		login, ok = s.LoginsByToken[credentials]
	} else {
		login, ok = s.LoginsBySessionID[credentials]
	}
	if !ok {
		return login, nil, ok
	}

	if s.clock.Since(login.CheckedAt) > s.TTL {
		return login, nil, false
	}

	userAuth, ok := s.RolesByLogin[login.Login]
	return login, userAuth, ok
}

// createUserRoles should only be called under lock.
func (s *AuthStorage) createUserRoles(credentials, login string, asToken bool) *UserRoles {
	var loginInfo *LoginInfo
	var ok bool

	if asToken {
		loginInfo, ok = s.LoginsByToken[credentials]
	} else {
		loginInfo, ok = s.LoginsBySessionID[credentials]
	}

	if !ok {
		loginInfo = &LoginInfo{Login: login, CheckedAt: s.clock.Now()}
		if asToken {
			s.LoginsByToken[credentials] = loginInfo
		} else {
			s.LoginsBySessionID[credentials] = loginInfo
		}
	}

	loginInfo.CheckedAt = s.clock.Now()

	userRoles, ok := s.RolesByLogin[login]
	if !ok {
		newRoles := make(UserRoles)
		s.RolesByLogin[login] = &newRoles
		userRoles = &newRoles
	}

	return userRoles
}

func (s *AuthStorage) SetLogin(credentials string, login string, asToken bool) {
	s.lock.Lock()
	defer s.lock.Unlock()
	s.createUserRoles(credentials, login, asToken)
}

func (s *AuthStorage) GetLogin(credentials string, asToken bool) (string, bool) {
	s.lock.RLock()
	defer s.lock.RUnlock()

	login, _, ok := s.getUserRoles(credentials, asToken)
	if !ok {
		return "", ok
	}

	return login.Login, s.clock.Since(login.CheckedAt) <= s.TTL
}

func (s *AuthStorage) SetRole(credentials, login, role string, granted, asToken bool) {
	s.lock.Lock()
	defer s.lock.Unlock()

	userRoles := *s.createUserRoles(credentials, login, asToken)

	idmRole, ok := userRoles[role]
	if !ok {
		idmRole = &IDMRole{}
		userRoles[role] = idmRole
	}
	idmRole.Granted = granted
	idmRole.CheckedAt = s.clock.Now()
}

func (s *AuthStorage) GetRole(credentials, role string, asToken bool) (login string, granted bool, ok bool) {
	s.lock.RLock()
	defer s.lock.RUnlock()

	loginInfo, userRoles, ok := s.getUserRoles(credentials, asToken)
	if !ok {
		return "", false, ok
	}

	idmRole, ok := (*userRoles)[role]
	if !ok {
		return "", false, ok
	}

	return loginInfo.Login, idmRole.Granted, s.clock.Since(idmRole.CheckedAt) <= s.TTL
}

func newAuthStorage() *AuthStorage {
	return newAuthStorageWithClock(clockwork.NewRealClock())
}

func newAuthStorageWithClock(clock clockwork.Clock) *AuthStorage {
	return &AuthStorage{
		LoginsByToken:     make(map[string]*LoginInfo),
		LoginsBySessionID: make(map[string]*LoginInfo),
		RolesByLogin:      make(map[string]*UserRoles),
		TTL:               5 * time.Minute,
		clock:             clock,
		lock:              sync.RWMutex{},
	}
}
