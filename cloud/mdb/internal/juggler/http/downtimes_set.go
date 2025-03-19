package http

import (
	"context"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/auth/tvm"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/cloud/mdb/internal/juggler/http/generated/swagger/client/downtimes"
	"a.yandex-team.ru/cloud/mdb/internal/juggler/http/generated/swagger/models"
)

func (jc *jugglerClient) SetDowntimes(ctx context.Context, r juggler.Downtime) (string, error) {
	span, ctx := opentracing.StartSpanFromContext(ctx, "SetDowntimes")
	defer span.Finish()
	params := downtimes.NewV2DowntimesSetDowntimesParamsWithContext(ctx)
	reqFilters := make([]*models.V2DowntimesSetDowntimesParamsBodyFiltersItems, 0, len(r.Filters))
	for _, item := range r.Filters {
		reqFilters = append(reqFilters, &models.V2DowntimesSetDowntimesParamsBodyFiltersItems{
			Host:      item.Host,
			Namespace: item.Namespace,
			Service:   item.Service,
		})
	}

	params.SetSetDowntimeRequest(&models.V2DowntimesSetDowntimesParamsBody{
		Description: r.Description,
		EndTime:     float64(r.EndTime.Unix()),
		Filters:     reqFilters,
		DowntimeID:  r.DowntimeID,
	})
	header := tvm.FormatOAuthToken(jc.oauth.Unmask())
	params.WithAuthorization(&header)
	respOk, err := jc.api.Downtimes.V2DowntimesSetDowntimes(params)
	if err != nil {
		return "", err
	}

	payload := respOk.GetPayload()
	return payload.DowntimeID, nil
}
