package store

import (
	"time"

	"a.yandex-team.ru/cloud/dataplatform/steadyprofiler/internal/config"
	"a.yandex-team.ru/cloud/dataplatform/steadyprofiler/internal/domain"
	"a.yandex-team.ru/yt/go/yt"
	"a.yandex-team.ru/yt/go/yt/ythttp"
)

type ProfilerStore interface {
	ProfileBlob(timestamp time.Time, service string, profileType string, resourceID string) ([]byte, error)
	ListProfiles(from, to time.Time, service, resourceID, typ string) ([]domain.Profile, error)
	ListServices(from time.Time) (*domain.ServicesList, error)
}

func NewStore(cfg *config.Config) (ProfilerStore, error) {
	client, err := ythttp.NewClient(&yt.Config{
		Proxy: cfg.Proxy,
		Token: string(cfg.YTToken),
	})
	if err != nil {
		return nil, err
	}
	return NewYtStore(client, cfg.BaseURL), nil
}
