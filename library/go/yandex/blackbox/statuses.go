package blackbox

import "fmt"

type Status uint8

// User authorization statuses
const (
	StatusValid     Status = 0
	StatusNeedReset Status = 1
	StatusExpired   Status = 2
	StatusNoAuth    Status = 3
	StatusDisabled  Status = 4
	StatusInvalid   Status = 5
)

func (s Status) String() string {
	switch s {
	case StatusValid:
		return "VALID"
	case StatusNeedReset:
		return "NEED_RESET"
	case StatusExpired:
		return "EXPIRED"
	case StatusNoAuth:
		return "NOAUTH"
	case StatusDisabled:
		return "DISABLED"
	case StatusInvalid:
		return "INVALID"
	default:
		return fmt.Sprintf("UNKNOWN_%d", uint8(s))
	}
}
