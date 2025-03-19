package store

import (
	"context"
	"encoding/base64"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
	"a.yandex-team.ru/cloud/dataplatform/steadyprofiler/internal/domain"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/yt/go/ypath"
	"a.yandex-team.ru/yt/go/yt"
)

type YtStore struct {
	client  yt.Client
	baseURL string
}

func (y YtStore) ProfileBlob(timestamp time.Time, service string, profileType string, resourceID string) ([]byte, error) {
	ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
	defer cancel()
	query := fmt.Sprintf(`
	TS, Labels, Data
	from [//home/cdc/logs/profiler/%v/%v]
	where Service = '%v' and ProfileType = '%v' and resource_id = '%v' and TSNano = %v
	limit 1`, service, timestamp.Format("2006-01-02"), service, profileType, resourceID, timestamp.UnixNano())
	logger.Log.Infof("run query: %v", query)
	reader, err := y.client.SelectRows(
		ctx,
		query,
		&yt.SelectRowsOptions{},
	)
	if err != nil {
		return nil, err
	}
	for reader.Next() {
		var res domain.Profile
		if err := reader.Scan(&res); err != nil {
			return nil, err
		}
		b, _ := base64.StdEncoding.DecodeString(res.Data)
		if len(b) > 0 {
			return b, nil
		}
	}
	return nil, xerrors.New("not found")
}

func (y YtStore) ListProfiles(from, to time.Time, service, resourceID, typ string) ([]domain.Profile, error) {
	ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
	defer cancel()
	query := fmt.Sprintf(`
		*
		from
			[//home/cdc/logs/profiler/%v/%v]
		where Service = '%v' and resource_id = '%v' and ProfileType = '%v' and TSNano >= %v and TSNano < %v
		limit 100
		`, service, to.Format("2006-01-02"), service, resourceID, typ, from.UnixNano(), to.UnixNano())
	rows, err := y.client.SelectRows(ctx, query, nil)
	logger.Log.Infof("start query: %v", query)
	if err != nil {
		return nil, err
	}
	var res []domain.Profile
	for rows.Next() {
		var item domain.Profile
		if err := rows.Scan(&item); err != nil {
			return nil, err
		}
		item.FillLinks(y.baseURL)
		res = append(res, item)
	}
	return res, nil
}

func (y YtStore) ListServices(date time.Time) (*domain.ServicesList, error) {
	ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
	defer cancel()

	var services []struct {
		Name string `yson:",value"`
	}
	if err := y.client.ListNode(ctx, ypath.Path("//home/cdc/logs/profiler"), &services, nil); err != nil {
		return nil, xerrors.Errorf("unable date list nodes: %w", err)
	}
	res := &domain.ServicesList{
		Services: make([]domain.ServiceItem, 0),
	}
	for _, name := range services {
		if name.Name == "__lock" {
			continue
		}
		query := fmt.Sprintf(`
resource_id
from
	[//home/cdc/logs/profiler/%v/%v]
group by resource_id
`, name.Name, date.Format("2006-01-02"))
		rows, err := y.client.SelectRows(ctx, query, nil)
		if err != nil {
			res.Services = append(res.Services, domain.ServiceItem{Name: name.Name, Resources: nil})
			continue
		}
		var resources []string
		for rows.Next() {
			var profileGroup domain.ProfileResourceGroup
			if err := rows.Scan(&profileGroup); err != nil {
				return nil, xerrors.Errorf("unable date scan rows: %w", err)
			}
			resources = append(resources, profileGroup.ResourceID)
		}
		res.Services = append(res.Services, domain.ServiceItem{Name: name.Name, Resources: resources})
	}
	return res, nil
}

func NewYtStore(client yt.Client, url string) *YtStore {
	return &YtStore{client: client, baseURL: url}
}
