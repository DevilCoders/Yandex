package clickhouse

import (
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/chutil"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/models"
)

type Config struct {
	DataOpts DataOptions   `json:"data_opts" yaml:"data_opts"`
	DB       chutil.Config `json:"db" yaml:"db"`
}

type DataOptions struct {
	Resource map[string]ResourceOptions `json:"resource" yaml:"resource"`
}

type ResourceOptions struct {
	Table            string                `json:"table" yaml:"table"`
	ResourceIDColumn string                `json:"resource_id_column" yaml:"resource_id_column"`
	InstanceColumn   string                `json:"instance_column" yaml:"instance_column"`
	FilterColumns    []string              `json:"filter_columns" yaml:"filter_columns"`
	SeverityFilter   SeverityFilterOptions `json:"severity_filter" yaml:"severity_filter"`

	MessageColumn     string `json:"message_column" yaml:"message_column"`
	TimeColumn        string `json:"time_column" yaml:"time_column"`
	MillisecondColumn string `json:"millisecond_column" yaml:"millisecond_column"`
}

type SeverityModifier string

const (
	DefaultModifier      SeverityModifier = "default"
	DataTransferModifier SeverityModifier = "data_transfer_modifier"
)

type SeverityFilterOptions struct {
	Select   string           `json:"select" yaml:"select"`
	Column   string           `json:"column" yaml:"column"`
	Modifier SeverityModifier `json:"modifier" yaml:"modifier"`
}

func DefaultConfig() Config {
	return Config{
		DataOpts: DataOptions{
			Resource: map[string]ResourceOptions{
				models.LogSourceTypeClickhouse.Stringify(): {
					Table:            "mdb.clickhouse",
					ResourceIDColumn: "cluster",
					InstanceColumn:   "hostname",
					FilterColumns:    []string{"component", "query_id", "thread"},
					SeverityFilter: SeverityFilterOptions{
						Column: "severity",
					},
					MessageColumn:     "message",
					TimeColumn:        "timestamp",
					MillisecondColumn: "ms",
				},
				models.LogSourceTypeKafka.Stringify(): {
					Table:            "mdb.kafka",
					ResourceIDColumn: "cluster",
					InstanceColumn:   "hostname",
					FilterColumns:    []string{"origin"},
					SeverityFilter: SeverityFilterOptions{
						Column: "severity",
					},
					MessageColumn:     "message",
					TimeColumn:        "timestamp",
					MillisecondColumn: "ms",
				},
				models.LogSourceTypeTransfer.Stringify(): {
					Table:            "mdb.data_transfer_dp",
					ResourceIDColumn: "id",
					InstanceColumn:   "host",
					FilterColumns:    []string{"task_id"},
					SeverityFilter: SeverityFilterOptions{
						Select:   "multiIf(level = 'INFO', 'Info', level = 'WARN', 'Warning', level = 'ERROR', 'Error', level = 'DEBUG', 'Debug', 'Info')", // kinda ugly, but who cares
						Column:   "level",
						Modifier: DataTransferModifier,
					},
					MessageColumn:     "concat(msg, if(error is not null, '\\n error ' || error, ''))",                            // same here
					MillisecondColumn: "toUnixTimestamp64Milli(_timestamp) - toUnixTimestamp64Milli(toStartOfSecond(_timestamp))", // and here
					TimeColumn:        "_timestamp",                                                                               // well, at least this is OK
				},
			},
		},
		DB: chutil.Config{
			DB:       "mdb",
			User:     "dbaas_reader",
			Secure:   true,
			Compress: true,
			CAFile:   "/opt/yandex/allCAs.pem",
		},
	}
}

func (d DataOptions) ParamsBySourceType(sourceType models.LogSourceType) ([]interface{}, error) {
	sourceSpecific, ok := d.Resource[sourceType.Stringify()]
	if !ok {
		return nil, semerr.InvalidInputf("unsupported source type %q", sourceType.Stringify())
	}

	severitySelectExpression := sourceSpecific.SeverityFilter.Select
	if severitySelectExpression == "" {
		severitySelectExpression = sourceSpecific.SeverityFilter.Column
	}
	return []interface{}{
		sourceSpecific.ResourceIDColumn,
		sourceSpecific.InstanceColumn,
		sourceSpecific.TimeColumn,
		sourceSpecific.MillisecondColumn,
		sourceSpecific.Table,
		severitySelectExpression,
		sourceSpecific.MessageColumn,
	}, nil

}
