package katan

import (
	"strconv"

	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
)

// SentryTagsFromRollout create tags for sentry from given rollout
func SentryTagsFromRollout(r katandb.Rollout) map[string]string {
	ret := map[string]string{
		"rollout_id": strconv.FormatInt(r.ID, 10),
	}
	if r.ScheduleID.Valid {
		ret["schedule_id"] = strconv.FormatInt(r.ScheduleID.Int64, 10)
	}
	if len(r.CreatedBy) > 0 {
		ret["user"] = r.CreatedBy
	}
	return ret
}
