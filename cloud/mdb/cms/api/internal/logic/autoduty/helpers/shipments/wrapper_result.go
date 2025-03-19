package shipments

import deployModels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"

type WrapperWaitResult struct {
	Failed   []deployModels.Shipment
	Progress []deployModels.Shipment
	Success  []deployModels.Shipment
	Timeout  []deployModels.Shipment
	Skipped  skippedHosts
}

type WrapperCreateResult struct {
	Success     []string
	NotInDeploy []string
	NotCreated  []string
	NotHA       []string
	Error       error
}

type skippedHosts struct {
	timeout  []string
	notFound []string
}

func (r *WrapperCreateResult) AllCreatedOk() bool {
	return r.Error == nil && len(r.NotCreated) == 0
}

func (r *WrapperCreateResult) AddSuccess(s string) {
	r.Success = append(r.Success, s)
}

func (r *WrapperCreateResult) AddNotInDeploy(s string) {
	r.NotInDeploy = append(r.NotInDeploy, s)
}

func (r *WrapperCreateResult) AddNotCreated(s string) {
	r.NotCreated = append(r.NotCreated, s)
}

func (r *WrapperCreateResult) AddNotHA(s string) {
	r.NotHA = append(r.NotHA, s)
}

func (r WrapperWaitResult) IsSuccessful() bool {
	return len(r.Failed)+len(r.Progress)+len(r.Timeout) == 0
}
