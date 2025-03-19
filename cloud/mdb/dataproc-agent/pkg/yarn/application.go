package yarn

type ApplicationAttempt struct {
	AppAttemptID  string
	AMContainerID string
}

// Application holds information about yarn application
type Application struct {
	Name                string
	ID                  string
	ApplicationAttempts []ApplicationAttempt
	FinalStatus         ApplicationFinalStatus
	State               ApplicationState
	StartedTime         int
}
