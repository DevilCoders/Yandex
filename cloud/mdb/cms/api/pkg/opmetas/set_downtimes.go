package opmetas

var _ OpMeta = &SetDowntimesStepMeta{}

type SetDowntimesStepMeta struct {
	DowntimeIDs []string `json:"downtime_ids"`
}

func (m *SetDowntimesStepMeta) SaveDT(dID string) {
	m.DowntimeIDs = append(m.DowntimeIDs, dID)
}

func NewSetDowntimesStepMeta() *SetDowntimesStepMeta {
	return &SetDowntimesStepMeta{DowntimeIDs: []string{}}
}
