package http

import (
	"context"
	"time"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/cloud/mdb/internal/juggler/http/generated/swagger/client/downtimes"
	"a.yandex-team.ru/cloud/mdb/internal/juggler/http/generated/swagger/models"
)

func (jc *jugglerClient) GetDowntimes(ctx context.Context, r juggler.Downtime) ([]juggler.Downtime, error) {
	span, ctx := opentracing.StartSpanFromContext(ctx, "GetDowntimes")
	defer span.Finish()
	params := downtimes.NewV2DowntimesGetDowntimesParamsWithContext(ctx)
	reqFilters := make([]*models.V2DowntimesGetDowntimesParamsBodyFiltersItems, 0, len(r.Filters))
	for _, filter := range r.Filters {
		reqFilters = append(reqFilters, &models.V2DowntimesGetDowntimesParamsBodyFiltersItems{
			DowntimeID: filter.DowntimeID,
			Host:       filter.Host,
			Namespace:  filter.Namespace,
			Service:    filter.Service,
		})
	}
	params.WithGetDowntimesRequest(&models.V2DowntimesGetDowntimesParamsBody{
		Filters: reqFilters,
	})
	respOk, err := jc.api.Downtimes.V2DowntimesGetDowntimes(params)
	if err != nil {
		return nil, err
	}

	payload := respOk.GetPayload()

	var result []juggler.Downtime
	for _, item := range payload.Items {
		dt := juggler.Downtime{
			DowntimeID:  item.DowntimeID,
			Description: item.Description,
			EndTime:     time.Unix(int64(item.EndTime), 0),
		}
		for _, filter := range item.Filters {
			dt.Filters = append(dt.Filters, juggler.DowntimeFilter{
				Host:      filter.Host,
				Namespace: filter.Namespace,
			})
		}
		result = append(result, dt)
	}
	return result, nil
}
