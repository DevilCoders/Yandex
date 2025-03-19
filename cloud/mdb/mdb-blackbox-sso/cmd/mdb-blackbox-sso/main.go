package main

import (
	"fmt"
	"io/ioutil"
	"net/http"
	"os"

	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth/blackboxauth"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

// Config for auth service
type Config struct {
	Listen string              `yaml:"listen"`
	Auth   blackboxauth.Config `yaml:"auth"`
}

// AuthService - blackbox SSO provider
type AuthService struct {
	auth   httpauth.Blackbox
	logger log.Logger
}

func serviceFromConfig(config Config) (*AuthService, error) {
	logger, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		return nil, err
	}

	auth, err := blackboxauth.NewFromConfig(config.Auth, logger)
	if err != nil {
		return nil, err
	}

	return &AuthService{auth: auth, logger: logger}, nil
}

func (s *AuthService) blackboxAuth(w http.ResponseWriter, r *http.Request) {
	uinfo, err := s.auth.AuthUser(r.Context(), r)
	if err != nil {
		w.WriteHeader(http.StatusForbidden)
		s.logger.Errorf("Forbidden: %s", err)
		return
	}
	s.logger.Infof("Allowed access for %s", uinfo.Login)
	w.Header().Set("X-SSO-Login", uinfo.Login)
	w.WriteHeader(http.StatusOK)
}

// Run starts http service
func (s *AuthService) Run(listen string) {
	http.HandleFunc("/auth", s.blackboxAuth)
	err := http.ListenAndServe(listen, nil)
	if err != nil {
		panic(err)
	}
}

func main() {
	if len(os.Args) != 2 {
		fmt.Print("No config provided")
		os.Exit(1)
	}
	raw, err := ioutil.ReadFile(os.Args[1])
	if err != nil {
		panic(err)
	}
	var config Config
	err = yaml.Unmarshal(raw, &config)
	if err != nil {
		panic(err)
	}
	service, err := serviceFromConfig(config)
	if err != nil {
		panic(err)
	}
	service.Run(config.Listen)
}
