package types

import (
	"database/sql/driver"
	"encoding/json"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type Labels map[string]string

func (l Labels) Value() (driver.Value, error) {
	bytes, err := json.Marshal(l)
	if err != nil {
		return nil, err
	}
	// https://github.com/jackc/pgtype/issues/45
	return string(bytes), nil
}

func (l Labels) Scan(src interface{}) error {
	data, ok := src.([]byte)
	if !ok {
		return xerrors.Errorf(errUnableToCastSourceToBytes, src)
	}

	if err := json.Unmarshal(data, &l); err != nil {
		return xerrors.Errorf(errUnableToUnmarshalJSON, err)
	}

	return nil
}
