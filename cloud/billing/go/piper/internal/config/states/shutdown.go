package states

import (
	"errors"
	"strings"
)

func (m *Manager) Shutdown() error {
	m.mu.Lock()
	defer m.mu.Unlock()

	if m.closed {
		return errors.New("manager already closed")
	}
	m.closed = true

	errs := shutdownErrors{}
	for i := range m.closers {
		c := m.closers[len(m.closers)-i-1]
		if err := c(); err != nil {
			errs = append(errs, err)
		}
	}
	if len(errs) > 0 {
		return errs
	}
	return nil
}

type shutdownErrors []error

func (e shutdownErrors) Error() string {
	bld := strings.Builder{}
	_, _ = bld.WriteString("shutdown errors: ")
	for i, e := range e {
		if i > 0 {
			_, _ = bld.WriteString("; ")
		}
		_, _ = bld.WriteString(e.Error())
	}
	return bld.String()
}
