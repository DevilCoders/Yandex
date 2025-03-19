package main

import (
	"database/sql"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/internal/x/net/dial"
	l "a.yandex-team.ru/library/go/core/log"
)

type database struct {
	name   string
	config DBConfig
	pool   *sql.DB
}

const passOption = "password="

func excludePassword(connStr string) string {
	iPassBegin := strings.Index(connStr, passOption)
	if iPassBegin < 0 {
		return connStr
	}
	iPassBegin += len(passOption)
	var tail string
	iPassEnd := strings.Index(connStr[iPassBegin:], " ")
	if iPassEnd >= 0 {
		tail = connStr[iPassBegin+iPassEnd:]
	}
	return connStr[0:iPassBegin] + "***" + tail
}

func createPool(logger l.Logger, connStr *string, wait bool) (*sql.DB, error) {
	bf := []l.Field{
		l.String("func", "createPool"),
		l.String("connection_string", excludePassword(*connStr)),
	}

	configKey, err := pgutil.RegisterConfigForConnString(*connStr, dial.TCPConfig{})
	if err != nil {
		return nil, err
	}

	for {
		db, err := sql.Open("pgx", configKey)
		if err == nil {
			logger.Debug("connection pool created", bf...)
			db.SetConnMaxLifetime(time.Hour)
			db.SetMaxIdleConns(5)
			return db, err
		}

		if !wait {
			return nil, err
		}

		logger.Error("creating connection pool failed", append(bf[:],
			l.NamedError("error", err),
		)...)
		time.Sleep(time.Second)
		defer func() { _ = db.Close() }()
	}
}

func getPool(logger l.Logger, host *host) (*sql.DB, error) {
	var db *sql.DB
	var err error
	if host.connectionPool != nil {
		// Reuse already created connection pool if possible
		return host.connectionPool, nil
	}

	db, err = createPool(logger, &host.connStr, false)
	if err != nil {
		return nil, err
	}
	host.connectionPool = db
	return db, nil
}
