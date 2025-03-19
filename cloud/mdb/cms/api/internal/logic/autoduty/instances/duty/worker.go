package duty

import (
	"context"
	"time"
)

func (ad *AutoDuty) Run(ctx context.Context) {
	workDuration := 10 * time.Second
	ticker := time.NewTicker(workDuration)
	defer ticker.Stop()
	ad.log.Info("Init worker loop")

	ad.Iteration(ctx)
	for {
		select {
		case <-ticker.C:
			ad.Iteration(ctx)

		case <-ctx.Done():
			ad.log.Info("Stop worker loop")
			return
		}
	}
}
