package steps

import (
	"context"
	"strings"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/library/go/core/log"
)

type RemoveDowntimes struct {
	jglr juggler.API
}

func (s RemoveDowntimes) Name() string {
	return "remove downtimes"
}

func (s RemoveDowntimes) RunStep(_ context.Context, stepCtx *opcontext.OperationContext, _ log.Logger) RunResult {
	ids := stepCtx.State().DowntimesStep.DowntimeIDs
	if len(ids) > 0 {
		return continueWithMessageFmt("downtimes are not removed (MDB-12376): %s", strings.Join(ids, ", "))
	} else {
		return continueWithMessage("there are no downtimes")
	}
}

func NewRemoveDowntimes(jglr juggler.API) RemoveDowntimes {
	return RemoveDowntimes{
		jglr: jglr,
	}
}
