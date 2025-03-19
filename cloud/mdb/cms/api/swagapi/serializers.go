package swagapi

import (
	swagmodel "a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

func stringsToFqnds(str []string) []swagmodel.Fqdn {
	var result []swagmodel.Fqdn
	for _, h := range str {
		result = append(result, swagmodel.Fqdn(h))
	}
	return result
}

func FQDNsToStrings(fqdns []swagmodel.Fqdn) []string {
	var result []string
	for _, f := range fqdns {
		result = append(result, string(f))
	}
	return result
}

func serializeStatus(st models.RequestStatus) (swagmodel.TaskStatus, error) {
	switch st {
	case models.StatusInProcess:
		return swagmodel.TaskStatusInDashProcess, nil
	case models.StatusRejected:
		return swagmodel.TaskStatusRejected, nil
	case models.StatusOK:
		return swagmodel.TaskStatusOk, nil
	default:
		return swagmodel.TaskStatusRejected, semerr.NotImplementedf("unsupported request status %v", st)
	}
}

func respReqStatus(r models.ManagementRequest) (*swagmodel.ManagementRequestStatusResponse, error) {
	st, err := serializeStatus(r.Status)
	if err != nil {
		return nil, err
	}
	extID := swagmodel.ManagementRequestID(r.ExtID)
	return &swagmodel.ManagementRequestStatusResponse{
		Hosts:   stringsToFqnds(r.Fqnds),
		ID:      &extID,
		Message: r.ResolveExplanation,
		Status:  &st,
	}, nil
}

func respListReqStatus(reqs []models.ManagementRequest) (*swagmodel.TasksResultsArray, error) {
	var resp swagmodel.TasksResultsArray
	resp.Result = make([]*swagmodel.ManagementRequestStatusResponse, 0, len(reqs))
	for _, r := range reqs {
		rs, err := respReqStatus(r)
		if err != nil {
			return nil, err
		}
		resp.Result = append(resp.Result, rs)
	}
	return &resp, nil
}
