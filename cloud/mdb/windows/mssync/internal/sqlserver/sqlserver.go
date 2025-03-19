package sqlserver

import (
	"database/sql"
	"fmt"
	"net/url"
	"time"

	utils "a.yandex-team.ru/cloud/mdb/windows/mssync/internal/utils"
)

type HealthState int

const (
	HealthStateClean   = 1
	HealthStateWarning = 2
	HealthStateError   = 3
	HealthStateUnknown = 0
)

var HealthStatetoString = map[HealthState]string{
	HealthStateUnknown: "UNKNOWN",
	HealthStateClean:   "CLEAN",
	HealthStateWarning: "WARNING",
	HealthStateError:   "ERROR",
}

type FailureConditionLevel int

const (
	FCLNone                 = 0
	FCLServerDown           = 1
	FCLServerUnresponsive   = 2
	FCLCriticalServerErrors = 3
	FCLModerateServerErrors = 4
	FCLAnyError             = 5
)

var FCLtoString = map[FailureConditionLevel]string{
	FCLNone:                 "None",
	FCLServerDown:           "ServerDown",
	FCLServerUnresponsive:   "ServerUnresponsive",
	FCLCriticalServerErrors: "CriticalServerErrors",
	FCLModerateServerErrors: "ModerateServerErrors",
	FCLAnyError:             "AnyQualifiedError",
}

const (
	AGRolePrimary              = "PRIMARY"
	AGRoleSecondary            = "SECONDARY"
	AGRoleResolving            = "RESOLVING"
	HealthItemSystem           = "system"
	HealthItemResources        = "resource"
	HealthItemsQueryProcessing = "query_processing"
)
const TimeSQLServerFormat = "Jan 02, 2006 03:04:05 PM"

type SQLServer struct {
	HostName     string
	InstanceName string
	Version      string
	Edition      string
	AGs          []AG
}

type InstanceHealth struct {
	SystemState               HealthState
	SystemHealthData          string
	ResourceState             HealthState
	ResourceHealthData        string
	QueryProcessingState      HealthState
	QueryProcessingHealthData string
}

type InstanceHealthItem struct {
	Name  string
	State HealthState
	Data  string
}

type AG struct {
	Name      string
	Databases []Database
	Role      string
}

type Database struct {
	Name  string
	State string
}

func (ss *SQLServer) GetSQLServerMetadata(db *sql.DB) error {
	var hostName string
	var instanceName string
	var version string
	var edition string
	err := db.QueryRow(QueryGetSQLServerMetadata).Scan(&hostName, &instanceName, &version, &edition)
	if err != nil {
		return err
	}
	ss.HostName = hostName
	ss.InstanceName = instanceName
	ss.Edition = edition
	ss.Version = version
	return nil
}

func (ss *SQLServer) LoadAGs(db *sql.DB) error {
	rows, err := db.Query(QueryGetAGs)
	if err != nil {
		return err
	}
	var ags []AG
	for rows.Next() {
		var ag AG
		var agName string
		err := rows.Scan(&agName)
		if err != nil {
			return err
		}
		ag.Name = agName
		err = ag.LoadAGRole(db)
		if err != nil {
			return fmt.Errorf("error loading AG roles: %v", err)
		}
		ags = append(ags, ag)
	}
	ss.AGs = ags
	err = ss.LoadAllAGDatabases(db)
	if err != nil {
		return fmt.Errorf("error loading AG databases: %v", err)
	}
	return nil
}

func (ag *AG) LoadAGDatabases(db *sql.DB) error {
	rows, err := db.Query(QueryGetAGDatabases, sql.Named("ag_name", ag.Name))
	if err != nil {
		return err
	}
	var databases []Database
	for rows.Next() {
		var database Database
		err := rows.Scan(&database.Name, &database.State)
		if err != nil {
			return err
		}
		databases = append(databases, database)
	}
	ag.Databases = databases
	return nil
}

func (ss *SQLServer) LoadAllAGDatabases(db *sql.DB) error {
	var agName string
	agIndex := make(map[string]int, len(ss.AGs))

	for n, ag := range ss.AGs {
		agIndex[ag.Name] = n
	}

	rows, err := db.Query(QueryGetAllAGDatabases)
	if err != nil {
		return err
	}
	for rows.Next() {
		var database Database
		err := rows.Scan(&agName, &database.Name, &database.State)
		if err != nil {
			return err
		}
		ss.AGs[agIndex[agName]].Databases = append(ss.AGs[agIndex[agName]].Databases, database)
	}
	return nil
}

func (ag *AG) LoadAGRole(db *sql.DB) error {
	var row string
	if err := db.QueryRow(QueryGetAGRole, sql.Named("ag_name", ag.Name)).Scan(&row); err != nil {
		return err
	}
	ag.Role = row
	return nil
}

func GetSQLServer(db *sql.DB) (SQLServer, error) {
	var sqlServer SQLServer
	err := sqlServer.GetSQLServerMetadata(db)
	if err != nil {
		return SQLServer{}, fmt.Errorf("error getting instance metadata: %v", err)
	}
	err = sqlServer.LoadAGs(db)
	if err != nil {
		return SQLServer{}, fmt.Errorf("error loading AGs: %v", err)
	}
	return sqlServer, nil
}

func (ss *SQLServer) IsAlive(failureConditionLevel int, db *sql.DB) (bool, InstanceHealth, error) {
	rows, err := db.Query(QueryGetInstanceHealth)
	if err != nil {
		return false, InstanceHealth{}, nil
	}
	var Health InstanceHealth
	var item InstanceHealthItem
	for rows.Next() {
		err := rows.Scan(&item.Name, &item.State, &item.Data)
		if err != nil {
			return false, InstanceHealth{}, nil
		}
		switch item.Name {
		case HealthItemSystem:
			Health.SystemState = item.State
			Health.SystemHealthData = item.Data
		case HealthItemResources:
			Health.ResourceState = item.State
			Health.ResourceHealthData = item.Data
		case HealthItemsQueryProcessing:
			Health.QueryProcessingState = item.State
			Health.QueryProcessingHealthData = item.Data
		}
	}
	var failureLevel int
	if Health.QueryProcessingState == HealthStateError || Health.ResourceState == HealthStateError || Health.SystemState == HealthStateError {
		failureLevel = FCLAnyError
	}
	if Health.ResourceState == HealthStateError || Health.SystemState == HealthStateError {
		failureLevel = FCLModerateServerErrors
	}
	if Health.SystemState == HealthStateError {
		failureLevel = FCLCriticalServerErrors
	}

	if failureLevel >= failureConditionLevel {
		return false, Health, nil
	}
	return true, Health, nil
}

func (ss *SQLServer) IsHADRSplit() (bool, int, int) {
	var numPrimaries int
	var numSecondaries int
	for _, ag := range ss.AGs {
		if ag.Role == AGRolePrimary {
			numPrimaries++
		}
		if ag.Role == AGRoleSecondary {
			numSecondaries++
		}
	}
	if numPrimaries > 0 && numSecondaries > 0 && (numPrimaries+numSecondaries) == len(ss.AGs) {
		return true, numPrimaries, numSecondaries
	}
	return false, numPrimaries, numSecondaries
}

func (ss *SQLServer) GetRoleChangeTimes(db *sql.DB) (time.Time, time.Time, error) {
	var promotionTime time.Time
	var demotionTime time.Time
	err := db.QueryRow(QueryGetRoleChangeTimes).Scan(&demotionTime, &promotionTime)
	if err != nil {
		return demotionTime, promotionTime, err
	}
	return demotionTime, promotionTime, nil
}

func (ss *SQLServer) Promote(db *sql.DB, log utils.LoggerType) error {
	for _, ag := range ss.AGs {
		if ag.Role == AGRoleSecondary {
			log.InfoLogger.Printf("Promoting AG [%s]...", ag.Name)
			err := ag.Promote(db)
			log.InfoLogger.Printf("AG [%s] promoted.", ag.Name)
			if err != nil {
				return err
			}
		}
	}
	return nil
}

func (ag *AG) Promote(db *sql.DB) error {
	agName := url.QueryEscape(ag.Name)
	query := fmt.Sprintf(QueryAGPromote, agName)
	_, err := db.Exec(query)
	if err != nil {
		return err
	}
	return nil
}

func (ss *SQLServer) IsPreferredReplica(db *sql.DB) (bool, error) {
	var a string
	var is_local bool
	err := db.QueryRow(QueryGetReplicas).Scan(&a, &is_local)
	if err != nil {
		return false, err
	}
	if is_local {
		return true, nil
	} else {
		return false, nil
	}
}
