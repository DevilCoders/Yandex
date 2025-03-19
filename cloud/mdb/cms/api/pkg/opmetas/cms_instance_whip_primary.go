package opmetas

import (
	cmsv1 "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
)

var _ OpMeta = &CmsInstanceWhipPrimaryMeta{}

type OperationIDStatus struct {
	ID     string                         `json:"id"`
	Status cmsv1.InstanceOperation_Status `json:"status"`
}

type OperationIDInfo map[string]OperationIDStatus

type CmsInstanceWhipPrimaryMeta struct {
	OperationIDs OperationIDInfo `json:"operationID"`
}

func (sm *CmsInstanceWhipPrimaryMeta) SetOperationInfo(fqdn string, ID string, Status cmsv1.InstanceOperation_Status) {
	_, ok := sm.OperationIDs[fqdn]
	if ok {
		delete(sm.OperationIDs, fqdn)
	}
	sm.OperationIDs[fqdn] = OperationIDStatus{ID, Status}

}

func (sm *CmsInstanceWhipPrimaryMeta) GetOperationIDs() OperationIDInfo {
	return sm.OperationIDs
}

func NewCmsInstanceWhipPrimaryMeta() *CmsInstanceWhipPrimaryMeta {
	return &CmsInstanceWhipPrimaryMeta{
		OperationIDs: OperationIDInfo{},
	}
}
