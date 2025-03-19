package salt

import "time"

type Job struct {
	ID   string
	Name string
	Args []string
}

type Result struct {
	ExitCode   int
	Error      error
	Stdout     string
	Stderr     string
	FinishedAt time.Time
}

func (r Result) Done() bool {
	return !r.FinishedAt.IsZero()
}

type Change struct {
	Job       Job
	PID       int
	StartedAt time.Time
	Progress  string
	Result    Result
}
