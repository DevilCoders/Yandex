package sqlmock

import (
	"database/sql"
	"database/sql/driver"
	"regexp"

	"github.com/DATA-DOG/go-sqlmock"

	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb/clickhouse"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Mock struct {
	DB *sql.DB

	mock sqlmock.Sqlmock

	data         []map[string]string
	columnsCount int
}

func New() (*Mock, error) {
	db, mock, err := sqlmock.New(sqlmock.QueryMatcherOption(sqlmock.QueryMatcherEqual))
	if err != nil {
		return nil, xerrors.Errorf("failed to initialize sqlmock driver for clickhouse backend of logsdb: %w", err)
	}

	return &Mock{DB: db, mock: mock}, nil
}

func (m *Mock) Reset() {
	m.data = nil
}

func (m *Mock) FillData(data []map[string]string) error {
	m.data = data

	m.columnsCount = 0
	for i, v := range data {
		if m.columnsCount == 0 {
			m.columnsCount = len(v)
			continue
		}

		if len(v) != m.columnsCount {
			return xerrors.Errorf("inconsistent row count for row %d", i)
		}
	}

	return nil
}

func (m *Mock) SetExpectations(cid string, lst logsdb.LogType, columnFilter []string, filter []sqlfilter.Term, limit, offset int64) error {
	query, whereParams, columns, err := clickhouse.BuildQueryAndParams(lst, columnFilter, filter, "log_time")
	if err != nil {
		return err
	}

	// Validate params len
	if len(columns) > m.columnsCount {
		return xerrors.Errorf("more than %d columns is not supported, got %d: %+v", m.columnsCount, len(columns), columns)
	}

	// We return one more than needed so that we can inform that there is more data
	limit++

	// Create expected rows
	rows := sqlmock.NewRows(columns)
	for row := offset; row < offset+limit; row++ {
		if int64(len(m.data)) <= row {
			break
		}

		var values []driver.Value
		for _, param := range columns {
			values = append(values, m.data[row][param])
		}

		rows.AddRow(values...)
	}

	// Set expectations
	if whereParams == nil {
		whereParams = make(map[string]interface{})
	}
	whereParams["cid"] = cid
	whereParams["from_time"] = sqlmock.AnyArg()
	whereParams["to_time"] = sqlmock.AnyArg()
	whereParams["from_ms"] = sqlmock.AnyArg()
	whereParams["to_ms"] = sqlmock.AnyArg()
	whereParams["offset"] = offset
	whereParams["limit"] = limit
	expectArgs := make([]driver.Value, 0, len(whereParams))
	expectQuery := regexp.MustCompile(`:\w+`).ReplaceAllStringFunc(query.Query, func(s string) string {
		expectArgs = append(expectArgs, whereParams[s[1:]])
		return "?"
	})
	m.mock.ExpectQuery(expectQuery).WithArgs(expectArgs...).WillReturnRows(rows)

	return nil
}

func (m *Mock) AssertExpectations() error {
	return m.mock.ExpectationsWereMet()
}
