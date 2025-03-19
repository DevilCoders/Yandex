package types

import "a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"

type RequestDecisionTuple struct {
	R models.ManagementRequest
	D models.AutomaticDecision
}
