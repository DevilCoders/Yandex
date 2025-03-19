package lockcluster

import "context"

//go:generate ../../../../../../../scripts/mockgen.sh Locker

type Holder string

const (
	WalleCMS    Holder = "mdb-cms-wall-e"
	InstanceCMS Holder = "mdb-cms-instance"
)

type Locker interface {
	LockCluster(ctx context.Context, fqdn string, taskID string, holder Holder) (*State, error)
	ReleaseCluster(ctx context.Context, state *State) error
}

type State struct {
	LockID  string `json:"lock_id" yaml:"lock_id"`
	IsTaken bool   `json:"is_taken" yaml:"is_taken"`
}
