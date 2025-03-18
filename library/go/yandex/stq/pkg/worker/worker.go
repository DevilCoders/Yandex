package worker

import (
	"time"
)

type Request struct {
	Cmd               string                 `bson:"cmd"`
	TaskID            string                 `bson:"task_id"`
	Args              []interface{}          `bson:"args"`
	Kwargs            map[string]interface{} `bson:"kwargs"`
	ExecTries         int                    `bson:"exec_tries"`
	RescheduleCounter int                    `bson:"reschedule_counter"`
	Eta               time.Time              `bson:"eta"`
}

type Response struct {
	Cmd      string `bson:"cmd"`
	TaskID   string `bson:"task_id"`
	Success  bool   `bson:"success"`
	ExecTime int64  `bson:"exec_time"`
}

type Worker interface {
	ProcessRequest(request Request) error
}
