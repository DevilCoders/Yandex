package leaderelection

import "context"

//go:generate ../../scripts/mockgen.sh LeaderElector

// LeaderElector is responsible for control leader flag and answer about who is a leader and is the instance a leader.
type LeaderElector interface {
	// IsLeader returns the flag about the current instance is leader or no.
	IsLeader(ctx context.Context) bool
}
