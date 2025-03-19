package http

import (
	"context"
	"fmt"
	"math"
	"net/url"
	"time"

	"github.com/go-openapi/strfmt"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/httputil/openapi"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/cloud/mdb/internal/juggler/http/generated/swagger/client"
	"a.yandex-team.ru/cloud/mdb/internal/juggler/http/generated/swagger/client/checks"
	"a.yandex-team.ru/cloud/mdb/internal/juggler/http/generated/swagger/client/raw_events"
	"a.yandex-team.ru/cloud/mdb/internal/juggler/http/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/library/go/core/log"
)

type Transport struct {
	TLS     httputil.TLSConfig     `json:"tls"`
	Logging httputil.LoggingConfig `json:"logging"`
}

type Config struct {
	Host      string        `json:"host" yaml:"host"`
	Token     secret.String `json:"token" yaml:"token"`
	Transport Transport     `json:"transport" yaml:"transport"`
}

func DefaultConfig() Config {
	return Config{
		Host: "https://juggler-api.search.yandex.net",
		Transport: Transport{
			Logging: httputil.DefaultLoggingConfig(),
		},
	}
}

type jugglerClient struct {
	host  string
	l     log.Logger
	api   *client.Juggler
	oauth secret.String
}

// NewClient create new client
func NewClient(config Config, l log.Logger) (juggler.API, error) {
	rt, err := httputil.DEPRECATEDNewTransport(
		config.Transport.TLS,
		config.Transport.Logging,
		l,
	)
	if err != nil {
		return nil, err
	}

	u, err := url.Parse(config.Host)
	if err != nil {
		return nil, err
	}

	crt := openapi.NewRuntime(
		u.Host,
		client.DefaultBasePath,
		[]string{u.Scheme},
		rt,
		l,
	)
	j := client.New(crt, strfmt.Default)

	return &jugglerClient{
		host:  config.Host,
		l:     l,
		api:   j,
		oauth: config.Token,
	}, nil
}

func (jc *jugglerClient) RawEvents(ctx context.Context, hosts, services []string) ([]juggler.RawEvent, error) {
	params := raw_events.NewV2EventsGetRawEventsParamsWithContext(ctx)
	filters := make([]*models.V2EventsGetRawEventsParamsBodyFiltersItems, 0, len(hosts)*len(services))
	for _, host := range hosts {
		for _, service := range services {
			filters = append(filters, &models.V2EventsGetRawEventsParamsBodyFiltersItems{
				Host:    host,
				Service: service,
			})
		}
	}
	if len(filters) == 0 {
		return nil, nil
	}

	var res []juggler.RawEvent
	var limit int64 = 200
	var offset int64 = 0

	for {
		params.SetGetRawEventsRequest(&models.V2EventsGetRawEventsParamsBody{
			Filters: filters,
			Limit:   limit,
			Offset:  offset,
			Sort: &models.V2EventsGetRawEventsParamsBodySort{
				Field: "HOST",
				Order: "DESC",
			},
		})
		respOk, err := jc.api.RawEvents.V2EventsGetRawEvents(params)
		if err != nil {
			return nil, err
		}

		payload := respOk.GetPayload()

		for _, item := range payload.Items {
			res = append(res, juggler.RawEvent{
				ReceivedTime: secToTime(item.ReceivedTime),
				Status:       fmt.Sprint(item.Status),
				Service:      item.Service,
				Host:         item.Host,
				Description:  item.Description,
			})
		}

		if payload.Total > (offset + limit) {
			offset += limit
		} else {
			break
		}
	}
	return res, nil
}

func (jc *jugglerClient) GetChecksState(ctx context.Context, host, service string) ([]juggler.CheckState, error) {
	args := checks.NewV2ChecksGetChecksStateParamsWithContext(ctx)
	args.SetGetCheckStateRequest(&models.V2ChecksGetChecksStateParamsBody{
		Filters: []*models.V2ChecksGetChecksStateParamsBodyFiltersItems{{
			Host:    host,
			Service: service,
		}},
	})
	respOk, err := jc.api.Checks.V2ChecksGetChecksState(args)
	if err != nil {
		return nil, err
	}

	payload := respOk.GetPayload()
	res := make([]juggler.CheckState, 0, len(payload.Items))
	for _, item := range payload.Items {
		res = append(res, juggler.CheckState{
			AggregationTime: secToTime(item.AggregationTime),
			ChangeTime:      secToTime(item.ChangeTime),
			Description:     item.Description,
			Host:            item.Host,
			Service:         item.Service,
			StateKind:       fmt.Sprint(item.StateKind),
			Status:          fmt.Sprint(item.Status),
		})
	}
	return res, nil
}

func secToTime(secF float64) time.Time {
	sec, dec := math.Modf(secF)
	return time.Unix(int64(sec), int64(dec*(1e9)))
}
