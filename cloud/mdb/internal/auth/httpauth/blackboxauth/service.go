package blackboxauth

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/auth/blackbox"
	"a.yandex-team.ru/cloud/mdb/internal/auth/blackbox/restapi"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/tvm"
	"a.yandex-team.ru/cloud/mdb/internal/auth/tvm/tvmtool"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
)

// interface satisfaction check
var _ httpauth.Authenticator = &BlackBoxAuth{}
var _ httpauth.Blackbox = &BlackBoxAuth{}

// BlackBoxAuth is responsible for request authentication using both blackbox and tvm
type BlackBoxAuth struct {
	tvmc              tvm.Client
	bbc               blackbox.Client
	alias             string
	baseHost          string
	l                 log.Logger
	cache             Cache
	loginListChecker  LoginListChecker
	oauthScopeChecker OAuthScopeChecker
}

// New creates new auth service
func New(bbc blackbox.Client, tvmc tvm.Client, alias string, baseHost string, addons ...interface{}) *BlackBoxAuth {
	var l log.Logger
	var cache Cache
	var loginListChecker LoginListChecker
	var oauthScopeChecker OAuthScopeChecker

	for _, addon := range addons {
		switch v := addon.(type) {
		case log.Logger:
			if l != nil {
				panic("Duplicate logger")
			}
			l = v
		case Cache:
			if cache != nil {
				panic("Duplicate cache")
			}
			cache = v
		case LoginListChecker:
			if loginListChecker != nil {
				panic("Duplicate loginListChecker")
			}
			loginListChecker = v
		case OAuthScopeChecker:
			if oauthScopeChecker != nil {
				panic("Duplicate oauthScopeChecker")
			}
			oauthScopeChecker = v
		default:
			panic(fmt.Sprintf("Got unexpected auth addon: %s", v))
		}
	}

	if l == nil {
		l = &nop.Logger{}
	}

	return &BlackBoxAuth{
		tvmc:              tvmc,
		bbc:               bbc,
		alias:             alias,
		baseHost:          baseHost,
		l:                 l,
		cache:             cache,
		loginListChecker:  loginListChecker,
		oauthScopeChecker: oauthScopeChecker,
	}
}

// Ping check that services are available
func (a *BlackBoxAuth) Ping(ctx context.Context) error {
	if err := a.tvmc.Ping(ctx); err != nil {
		return err
	}
	if err := a.bbc.Ping(ctx); err != nil {
		return err
	}
	return nil
}

// Config for constructor
type Config struct {
	Tvm            tvmtool.Config `yaml:"tvmtool"`
	BlackboxURI    string         `yaml:"blackbox_uri"`
	BlackboxAlias  string         `yaml:"blackbox_alias"`
	BaseHost       string         `yaml:"base_host"`
	UserScopes     []string       `yaml:"user_scopes"`
	LoginWhiteList []string       `yaml:"login_white_list"`
	CacheTTL       time.Duration  `yaml:"cache_ttl"`
	CacheSize      int64          `yaml:"cache_size"`
}

// NewFromConfig creates auth service from config
func NewFromConfig(config Config, l log.Logger) (*BlackBoxAuth, error) {
	tvmc, err := tvmtool.NewFromConfig(config.Tvm, l)
	if err != nil {
		return nil, err
	}

	bbc, err := restapi.New(config.BlackboxURI, l)
	if err != nil {
		return nil, err
	}

	var addons []interface{}

	addons = append(addons, l)

	if len(config.UserScopes) > 0 {
		addons = append(addons, &OAuthScopeListChecker{scopes: config.UserScopes})
	}

	if len(config.LoginWhiteList) > 0 {
		addons = append(addons, NewLoginSortedListChecker(config.LoginWhiteList))
	}

	if config.CacheTTL > 0 && config.CacheSize > 0 {
		addons = append(addons, NewCCache(config.CacheTTL, config.CacheSize))
	}

	return New(bbc, tvmc, config.BlackboxAlias, config.BaseHost, addons...), nil
}
