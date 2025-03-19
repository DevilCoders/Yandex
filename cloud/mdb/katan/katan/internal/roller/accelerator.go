package roller

import "fmt"

// RolloutStats is a current rollout stats
type RolloutStat struct {
	Done    int
	Running int
	Skipped int
	Errors  int
}

// Accelerator takes done, running counts and return whenever we should start another rollout
type Accelerator func(count RolloutStat) bool

func StrictLinearAccelerator(parallel int) Accelerator {
	if parallel <= 0 {
		panic(fmt.Errorf("parallel less than 1 is not supported: %d", parallel))
	}
	return func(c RolloutStat) bool {
		if c.Errors > 0 {
			return false
		}
		if c.Running >= parallel {
			return false
		}
		if c.Running < (c.Done-c.Skipped)*2 {
			return true
		}
		if c.Running == 0 {
			return true
		}
		return false
	}
}
