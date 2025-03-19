package yarn

// the final status of the application - reported by the application itself
// https://hadoop.apache.org/docs/current/hadoop-yarn/hadoop-yarn-site/ResourceManagerRest.html
type ApplicationFinalStatus int

const (
	FinalStatusUnknown ApplicationFinalStatus = iota
	FinalStatusUndefined
	FinalStatusSucceded
	FinalStatusFailed
	FinalStatusKilled
)

var stringToFinalStatus = map[string]ApplicationFinalStatus{
	"UNDEFINED": FinalStatusUndefined,
	"SUCCEEDED": FinalStatusSucceded,
	"FAILED":    FinalStatusFailed,
	"KILLED":    FinalStatusKilled,
}

func ToFinalStatus(str string) ApplicationFinalStatus {
	finalStatus, found := stringToFinalStatus[str]
	if found {
		return finalStatus
	} else {
		return FinalStatusUnknown
	}
}

func (finalStatus ApplicationFinalStatus) String() string {
	for k, v := range stringToFinalStatus {
		if v == finalStatus {
			return k
		}
	}
	return "UNKNOWN"
}
