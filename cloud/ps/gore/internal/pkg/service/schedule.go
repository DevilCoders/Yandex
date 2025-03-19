package service

import (
	"errors"
)

var (
	ErrMethodNotSupported = errors.New("'Method': Importing method not supported")
	ErrNoOwnerPresent     = errors.New("'Order': No 'lead' role found")
)

type Schedule struct { //Расписание
	Active     *bool             `json:"active,omitempty"` // VERIFY: is ptr
	Method     string            `json:"method,omitempty"` // VERIFY: enum //Однозначный выбор метода получения расписания
	Order      map[string]int    `json:"order,omitempty"`  // VERIFY: no //Определение пары имя_роли - приоритет
	KwArgs     map[string]string `json:"kwargs,omitempty"` // Deprecated
	File       *File             `json:"file,omitempty"`
	ABC        *ABC              `json:"abc,omitempty"`
	RoundRobin *RoundRobin       `json:"round_robin,omitempty"`
}

func (sch *Schedule) IsActive() bool {
	return *sch.Active
}

func (sch *Schedule) IsValid() error {
	if !methodLookup(sch.Method) {
		return ErrMethodNotSupported
	}

	if _, ok := sch.Order["lead"]; !ok {
		return ErrNoOwnerPresent
	}

	return nil
}

func methodLookup(m string) bool {
	for _, i := range importAvailable {
		if m == i {
			return true
		}
	}

	return false
}
