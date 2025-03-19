package optional

import (
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

type LabelsType map[string]string

type Labels struct {
	Labels LabelsType
	Valid  bool
}

func NewLabels(l LabelsType) Labels {
	return Labels{Labels: l, Valid: true}
}

func (o *Labels) Set(l LabelsType) {
	o.Valid = true
	o.Labels = l
}

func (o *Labels) Get() (LabelsType, error) {
	if !o.Valid {
		return nil, optional.ErrMissing
	}
	return o.Labels, nil
}

type MaintenanceWindow struct {
	MaintenanceWindow clusters.MaintenanceWindow
	Valid             bool
}

func NewMaintenanceWindow(mw clusters.MaintenanceWindow) MaintenanceWindow {
	return MaintenanceWindow{MaintenanceWindow: mw, Valid: true}
}

func (o *MaintenanceWindow) Set(mw clusters.MaintenanceWindow) {
	o.Valid = true
	o.MaintenanceWindow = mw
}

func (o *MaintenanceWindow) Get() (clusters.MaintenanceWindow, error) {
	if !o.Valid {
		return clusters.MaintenanceWindow{}, optional.ErrMissing
	}
	return o.MaintenanceWindow, nil
}
