package clusters

import "fmt"

type Health int

const (
	HealthUnknown Health = iota
	HealthDead
	HealthDegraded
	HealthAlive
)

var mapHealthToString = map[Health]string{
	HealthUnknown:  "Unknown",
	HealthDead:     "Dead",
	HealthDegraded: "Degraded",
	HealthAlive:    "Alive",
}

func (h Health) String() string {
	str, ok := mapHealthToString[h]
	if !ok {
		return fmt.Sprintf("UNKNOWN_CLUSTER_HEALTH_%d", h)
	}

	return str
}
