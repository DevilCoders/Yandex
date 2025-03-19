package godogutil

import (
	"fmt"
	"time"
)

func formatTestDuration(d time.Duration) string {
	return fmt.Sprintf("%d.%02d", int(d.Seconds()), d.Milliseconds()/10%100)
}
