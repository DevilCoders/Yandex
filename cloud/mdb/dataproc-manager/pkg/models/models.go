package models

import (
	"encoding/json"
	"fmt"
	"sort"
	"strings"

	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/health"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/service"
)

// ClusterHealth describes full cluster health report by services
type ClusterHealth struct {
	Cid            string
	Health         health.Health
	Services       map[service.Service]ServiceHealth
	Explanation    string
	HdfsInSafemode bool
	ReportCount    int64
	// timestamp of the earliest health check among all services
	UpdateTime int64
}

// NewClusterHealth creates cluster without health info
func NewClusterHealth(cid string) ClusterHealth {
	return ClusterHealth{
		Cid:      cid,
		Services: make(map[service.Service]ServiceHealth),
	}
}

// ServiceHealth cluster service health interface
type ServiceHealth interface {
	GetHealth() health.Health
	GetExplanation() string
}

// BasicHealthService is a basic realization of service health status info
type BasicHealthService struct {
	Health      health.Health
	Explanation string
}

// GetHealth is a return health realization for basic service
func (srv BasicHealthService) GetHealth() health.Health {
	return srv.Health
}

// GetExplanation returns string explaining service health status
func (srv BasicHealthService) GetExplanation() string {
	return srv.Explanation
}

// ServiceHbase describes health and status of Hbase service
type ServiceHbase struct {
	BasicHealthService
	Regions     int64
	Requests    int64
	AverageLoad float64
}

// ServiceHdfs describes health and status of Hbase service
type ServiceHdfs struct {
	BasicHealthService
	PercentRemaining        float64
	Used                    int64
	Free                    int64
	TotalBlocks             int64
	MissingBlocks           int64
	MissingBlocksReplicaOne int64
	Safemode                bool
}

// ServiceHive describes health and status of Hbase service
type ServiceHive struct {
	BasicHealthService
	QueriesSucceeded int64
	QueriesFailed    int64
	QueriesExecuting int64
	SessionsOpen     int64
	SessionsActive   int64
}

// MarshalBinary provides binary marshaling for redis
func (ch *ClusterHealth) MarshalBinary() ([]byte, error) {
	return json.Marshal(ch)
}

// UnmarshalBinary provides unmarshaling for redis
func (ch *ClusterHealth) UnmarshalBinary(data []byte) error {
	tempStruct := struct {
		Cid            string
		Health         health.Health
		Services       map[service.Service]json.RawMessage
		Explanation    string
		HdfsInSafemode bool
		ReportCount    int64
		UpdateTime     int64
	}{}
	err := json.Unmarshal(data, &tempStruct)
	if err != nil {
		return err
	}
	ch.Cid = tempStruct.Cid
	ch.Health = tempStruct.Health
	ch.Services = make(map[service.Service]ServiceHealth, len(tempStruct.Services))
	ch.Explanation = tempStruct.Explanation
	ch.HdfsInSafemode = tempStruct.HdfsInSafemode
	ch.ReportCount = tempStruct.ReportCount
	ch.UpdateTime = tempStruct.UpdateTime
	for serviceType, message := range tempStruct.Services {
		switch serviceType {
		case service.Hbase:
			serviceInfo := ServiceHbase{}
			err = json.Unmarshal(message, &serviceInfo)
			ch.Services[serviceType] = serviceInfo
		case service.Hdfs:
			serviceInfo := ServiceHdfs{}
			err = json.Unmarshal(message, &serviceInfo)
			ch.Services[serviceType] = serviceInfo
		case service.Hive:
			serviceInfo := ServiceHive{}
			err = json.Unmarshal(message, &serviceInfo)
			ch.Services[serviceType] = serviceInfo
		default:
			serviceInfo := BasicHealthService{}
			err = json.Unmarshal(message, &serviceInfo)
			ch.Services[serviceType] = serviceInfo
		}
		if err != nil {
			return err
		}
	}
	return nil
}

// HasService checks if cluster has service
func (ch *ClusterHealth) HasService(serviceType service.Service) bool {
	_, ok := ch.Services[serviceType]
	return ok
}

// ServiceHealth returns health for provided service name
func (ch *ClusterHealth) ServiceHealth(serviceType service.Service) health.Health {
	service, ok := ch.Services[serviceType]
	if !ok {
		return health.Disabled
	}
	return service.GetHealth()
}

// AllServicesAlive checks that all services of cluster are alive
func (ch *ClusterHealth) AllServicesAlive() bool {
	for _, service := range ch.Services {
		if service.GetHealth() != health.Alive {
			return false
		}
	}

	return true
}

// AllServicesDead checks that all services of cluster are dead
func (ch *ClusterHealth) AllServicesDead() bool {
	for _, service := range ch.Services {
		if service.GetHealth() != health.Dead {
			return false
		}
	}

	return true
}

// ExplainServicesHealth explains cluster health status deduced from health status of each service
func (ch *ClusterHealth) ExplainServicesHealth() string {
	explanations := make([]string, 0, len(ch.Services))
	for service, serviceHealth := range ch.Services {
		if serviceHealth.GetHealth() != health.Alive {
			serviceExplanation := fmt.Sprintf("%s is %s", service.String(),
				serviceHealth.GetHealth().String())
			if serviceHealth.GetExplanation() != "" {
				serviceExplanation += fmt.Sprintf(" (%s)", serviceHealth.GetExplanation())
			}
			explanations = append(explanations, serviceExplanation)
		}
	}
	// sort explanations by service name in order to get stable result
	sort.Strings(explanations)

	return strings.Join(explanations, ", ")
}

// DeduceHealth returns health status of a cluster based on a services health
func (ch *ClusterHealth) DeduceHealth() (health.Health, string) {
	if ch.AllServicesDead() {
		return health.Dead, "all services are Dead: " + ch.ExplainServicesHealth()
	}
	if ch.ServiceHealth(service.Hdfs) == health.Dead || ch.ServiceHealth(service.Yarn) == health.Dead {
		return health.Dead, "some critical services are Dead: " + ch.ExplainServicesHealth()
	}
	if ch.AllServicesAlive() {
		return health.Alive, ""
	}
	return health.Degraded, "some services are not Alive: " + ch.ExplainServicesHealth()
}

// HostHealth describes host health
type HostHealth struct {
	Fqdn     string
	Health   health.Health
	Services map[service.Service]ServiceHealth
}

// ServiceHbaseNode describes health and status of Hbase node
type ServiceHbaseNode struct {
	BasicHealthService
	Requests      int64
	HeapSizeMb    int64
	MaxHeapSizeMb int64
}

// ServiceHdfsNode describes health and status of Hbase node
type ServiceHdfsNode struct {
	BasicHealthService
	Used      int64
	Remaining int64
	Capacity  int64
	NumBlocks int64
	State     string
}

// ServiceYarnNode describes health and status of Yarn node
type ServiceYarnNode struct {
	BasicHealthService
	State             string
	NumContainers     int64
	UsedMemoryMb      int64
	AvailableMemoryMb int64
}

// MarshalBinary provides binary marshaling for redis
func (hh *HostHealth) MarshalBinary() ([]byte, error) {
	return json.Marshal(hh)
}

// UnmarshalBinary provides unmarshaling for redis
func (hh *HostHealth) UnmarshalBinary(data []byte) error {
	tempStruct := struct {
		Fqdn     string
		Health   health.Health
		Services map[service.Service]json.RawMessage
	}{}
	err := json.Unmarshal(data, &tempStruct)
	if err != nil {
		return err
	}
	hh.Fqdn = tempStruct.Fqdn
	hh.Health = tempStruct.Health
	hh.Services = make(map[service.Service]ServiceHealth, len(tempStruct.Services))
	for serviceType, message := range tempStruct.Services {
		switch serviceType {
		case service.Hbase:
			serviceInfo := ServiceHbaseNode{}
			err = json.Unmarshal(message, &serviceInfo)
			hh.Services[serviceType] = serviceInfo
		case service.Hdfs:
			serviceInfo := ServiceHdfsNode{}
			err = json.Unmarshal(message, &serviceInfo)
			hh.Services[serviceType] = serviceInfo
		case service.Yarn:
			serviceInfo := ServiceYarnNode{}
			err = json.Unmarshal(message, &serviceInfo)
			hh.Services[serviceType] = serviceInfo
		default:
			serviceInfo := BasicHealthService{}
			err = json.Unmarshal(message, &serviceInfo)
			hh.Services[serviceType] = serviceInfo
		}
		if err != nil {
			return err
		}
	}
	return nil
}

// HasService checks if cluster has service
func (hh *HostHealth) HasService(serviceType service.Service) bool {
	_, ok := hh.Services[serviceType]
	return ok
}

// AllServicesAlive checks that all services of a host are alive
func (hh *HostHealth) AllServicesAlive() bool {
	for _, serviceHealth := range hh.Services {
		if serviceHealth.GetHealth() != health.Alive && serviceHealth.GetHealth() != health.Decommissioning {
			return false
		}
	}
	return true
}

// AllServicesDead checks that all services of a host are dead
func (hh *HostHealth) AllServicesDead() bool {
	for _, serviceHealth := range hh.Services {
		if serviceHealth.GetHealth() != health.Dead {
			return false
		}
	}
	return true
}

// AllServicesDecommissioned checks that YARN and HDFS services of a host are decommissioned
func (hh *HostHealth) AllServicesDecommissioned() bool {
	if hostHealth, exists := hh.Services[service.Yarn]; exists {
		if hostHealth.GetHealth() != health.Decommissioned {
			return false
		}
	}
	if hostHealth, exists := hh.Services[service.Hdfs]; exists {
		if hostHealth.GetHealth() != health.Decommissioned {
			return false
		}
	}
	return true
}

// ServiceHealth gets service health status for host
func (hh *HostHealth) ServiceHealth(serviceType service.Service) health.Health {
	serviceHealth, ok := hh.Services[serviceType]
	if !ok {
		return health.Disabled
	}
	return serviceHealth.GetHealth()
}

// DeduceHealth gives health status of a host based on a services health
func (hh *HostHealth) DeduceHealth() health.Health {
	if hh.AllServicesAlive() {
		return health.Alive
	}
	if hh.AllServicesDead() {
		return health.Dead
	}
	if hh.AllServicesDecommissioned() {
		return health.Decommissioned
	}
	return health.Degraded
}

// NewHostWithServices creates host health struct with enabled service list
func NewHostWithServices(fqdn string, servicesList []service.Service) HostHealth {
	services := make(map[service.Service]ServiceHealth, len(servicesList))
	for _, serviceName := range servicesList {
		services[serviceName] = BasicHealthService{Health: health.Dead}
	}
	return HostHealth{
		Fqdn:     fqdn,
		Services: services,
	}
}

// NewHostUnknownHealth creates host health struct with unknown status
func NewHostUnknownHealth() HostHealth {
	return HostHealth{
		Health: health.Unknown,
	}
}

type DecommissionHosts struct {
	Timeout   int64
	YarnHosts []string
	HdfsHosts []string
}

// MarshalBinary provides binary marshaling for cluster topology
func (tp *DecommissionHosts) MarshalBinary() ([]byte, error) {
	return json.Marshal(tp)
}

func (tp *DecommissionHosts) UnmarshalBinary(data []byte) error {
	return json.Unmarshal(data, tp)
}

type DecommissionStatus struct {
	YarnRequestedDecommissionHosts []string
	HdfsRequestedDecommissionHosts []string
}

func (tp *DecommissionStatus) MarshalBinary() ([]byte, error) {
	return json.Marshal(tp)
}

func (tp *DecommissionStatus) UnmarshalBinary(data []byte) error {
	return json.Unmarshal(data, tp)
}
