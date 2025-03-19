package models

type TaskStatus string

func (os TaskStatus) IsTerminal() bool {
	return os == TaskStatusDone || os == TaskStatusFailed
}

const (
	TaskStatusPending TaskStatus = "PENDING"
	TaskStatusRunning TaskStatus = "RUNNING"
	TaskStatusDone    TaskStatus = "DONE"
	TaskStatusFailed  TaskStatus = "FAILED"
)
