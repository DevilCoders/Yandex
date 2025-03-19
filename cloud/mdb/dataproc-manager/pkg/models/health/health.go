package health

// Health is a health status enum
type Health int

const (
	// Unknown - can't get status
	Unknown Health = iota
	// Disabled - turned off by user
	Disabled
	// Dead - totally not working
	Dead
	// Degraded - partially not working
	Degraded
	// Alive - working
	Alive
	// Is in progress of node decommission
	Decommissioning
	// Is safe to stop/delete
	Decommissioned
)

func (health Health) String() string {
	names := [...]string{
		"Unknown",
		"Disabled",
		"Dead",
		"Degraded",
		"Alive",
		"Decommissioning",
		"Decommissioned",
	}
	return names[health]
}
