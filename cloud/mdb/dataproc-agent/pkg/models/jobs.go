package models

import "a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/yarn"

// JobState is dataproc job state
type JobState int

const (
	// initial job's state
	JobStateProvisioning JobState = iota
	// job is accepted for execution on hadoop cluster
	JobStatePending
	// job is running
	JobStateRunning
	// job finished with error
	JobStateError
	// job finished as expected
	JobStateDone
	// job cancelled
	JobStateCancelled
	// job is waiting for cancellation
	JobStateCancelling
)

// IsTerminal return true if it is final state of job, false otherwise
func (state JobState) IsTerminal() bool {
	switch state {
	case JobStateError, JobStateDone, JobStateCancelled:
		return true
	default:
		return false
	}
}

var jobStateToString = map[JobState]string{
	JobStateProvisioning: "Provisioning",
	JobStatePending:      "Pending",
	JobStateRunning:      "Running",
	JobStateError:        "Error",
	JobStateDone:         "Done",
	JobStateCancelled:    "Cancelled",
	JobStateCancelling:   "Cancelling",
}

func (state JobState) String() string {
	str, found := jobStateToString[state]
	if found {
		return str
	}
	return "Unknown"
}

type JobSpec interface {
	isJobSpec()
}

type JobInfo struct {
	ID    string
	State JobState
}

type Job struct {
	JobInfo
	Spec        JobSpec
	Application yarn.Application
}

// MapreduceJob holds parameters of map-reduce job
type MapreduceJob struct {
	MainJarFileURI string
	MainClass      string
	JarFileURIs    []string
	FileURIs       []string
	ArchiveURIs    []string
	Properties     map[string]string
	Args           []string
}

func (*MapreduceJob) isJobSpec() {}

// HiveJob holds parameters of hive job
type HiveJob struct {
	QueryFileURI      string
	QueryList         []string
	ContinueOnFailure bool
	ScriptVariables   map[string]string
	Properties        map[string]string
	JarFileURIs       []string
}

func (*HiveJob) isJobSpec() {}

// PysparkJob defines parameters of spark job written in python
type PysparkJob struct {
	MainPythonFileURI string
	PythonFileURIs    []string
	JarFileURIs       []string
	Packages          []string
	Repositories      []string
	ExcludePackages   []string
	FileURIs          []string
	ArchiveURIs       []string
	Properties        map[string]string
	Args              []string
}

func (*PysparkJob) isJobSpec() {}

// SparkJob defines parameters of spark job written in java/scala
type SparkJob struct {
	MainJarFileURI  string
	MainClass       string
	JarFileURIs     []string
	Packages        []string
	Repositories    []string
	ExcludePackages []string
	FileURIs        []string
	ArchiveURIs     []string
	Properties      map[string]string
	Args            []string
}

func (*SparkJob) isJobSpec() {}
