package yarn

// ApplicationState is yarn application state
type ApplicationState int

const (
	// job haven't submitted yet, or state haven't been requested
	StateUnknown ApplicationState = iota
	// client requested application id
	StateNew
	// client sent submit request, but job info is not saved yet
	StateNewSaving
	// job info saved to yarn state store
	StateSubmitted
	// job is scheduled for execution
	StateAccepted
	// job is running
	StateRunning
	// job finished as expected
	StateFinished
	// job finished with error
	StateFailed
	// client killed the job
	StateKilled
)

var stringToState = map[string]ApplicationState{
	"NEW":        StateNew,
	"NEW_SAVING": StateNewSaving,
	"SUBMITTED":  StateSubmitted,
	"ACCEPTED":   StateAccepted,
	"RUNNING":    StateRunning,
	"FINISHED":   StateFinished,
	"FAILED":     StateFailed,
	"KILLED":     StateKilled,
}

// ToState allow to convert string representation to ApplicationState object
func ToState(str string) ApplicationState {
	state, found := stringToState[str]
	if found {
		return state
	} else {
		return StateUnknown
	}
}

// String converts ApplicationState object to its string representation
func (state ApplicationState) String() string {
	for k, v := range stringToState {
		if v == state {
			return k
		}
	}
	return "UNKNOWN"
}
