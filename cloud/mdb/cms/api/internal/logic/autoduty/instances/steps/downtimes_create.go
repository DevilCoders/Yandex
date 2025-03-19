package steps

import (
	"context"
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/internal/fqdn"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type CreateDowntimes struct {
	jglr          juggler.API
	fqdnConverter fqdn.Converter
}

func (s CreateDowntimes) Name() string {
	return "create downtimes"
}

func (s CreateDowntimes) RunStep(ctx context.Context, stepCtx *opcontext.OperationContext, l log.Logger) RunResult {
	ids := stepCtx.State().DowntimesStep.DowntimeIDs
	if len(ids) > 0 {
		ctxlog.Debug(ctx, l, "Prolong downtimes", log.Strings("downtime ids", ids))
		filters := make([]juggler.DowntimeFilter, len(ids))
		for i, id := range ids {
			filters[i] = juggler.DowntimeFilter{DowntimeID: id}
		}
		downtimes, err := s.jglr.GetDowntimes(ctx, juggler.Downtime{
			Filters: filters,
		})
		if err != nil {
			return waitWithErrAndMessage(err, "could not get downtimes")
		}
		for _, dt := range downtimes {
			//dt.EndTime = dt.EndTime.Add(time.Hour) this code should be used after remove dowtimes method will be available
			dt.EndTime = time.Now().Add(time.Hour)
			_, err = s.jglr.SetDowntimes(ctx, dt)
			if err != nil {
				return waitWithErrAndMessage(err, "could not set downtime")
			}
		}
	} else {
		ctxlog.Debug(ctx, l, "State is empty, create new downtimes.")
		managedFQDN, err := s.fqdnConverter.UnmanagedToManaged(stepCtx.FQDN())
		if err != nil {
			return waitWithErrAndMessage(err, fmt.Sprintf("could not convert fqdn %s into managed fqdn", stepCtx.FQDN()))
		}
		dtr := juggler.NewSetDowntimeRequestByDuration(
			fmt.Sprintf("CMS %s operation %s", stepCtx.OperationType(), stepCtx.OperationID()),
			time.Hour,
			[]juggler.DowntimeFilter{{Host: managedFQDN}},
		)
		dtID, err := s.jglr.SetDowntimes(ctx, dtr)
		if err != nil {
			return waitWithErrAndMessage(err, "could not set downtime")
		}
		ids = append(ids, dtID)
		stepCtx.State().DowntimesStep.DowntimeIDs = ids
	}

	return continueWithMessageFmt("downtime ids: %s", strings.Join(ids, ", "))
}

func NewCreateDowntimes(jglr juggler.API, fqdnConverter fqdn.Converter) CreateDowntimes {
	return CreateDowntimes{
		jglr:          jglr,
		fqdnConverter: fqdnConverter,
	}
}
