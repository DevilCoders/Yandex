package states

import (
	"context"
	"sync"
)

type (
	StateObject         interface{}
	CriticalStateObject interface {
		HealthCheck(context.Context) error
	}
)

type Manager struct {
	names          []string
	healthCheckers map[string]healthChecker

	criticalHealthCheckers []string

	statsProviders map[string]statsProvider

	closers []closeCall
	closed  bool

	mu sync.RWMutex
}

func (m *Manager) Add(name string, so StateObject) *Manager {
	m.add(name, so, false)
	return m
}

func (m *Manager) AddCritical(name string, so CriticalStateObject) *Manager {
	m.add(name, so, true)
	return m
}

func (m *Manager) init() {
	if m.healthCheckers == nil {
		m.healthCheckers = make(map[string]healthChecker)
	}
	if m.statsProviders == nil {
		m.statsProviders = make(map[string]statsProvider)
	}
}

func (m *Manager) add(name string, so StateObject, critical bool) {
	m.mu.Lock()
	defer m.mu.Unlock()

	if m.closed {
		return
	}
	m.init()

	m.names = append(m.names, name)
	if hc := getHealthChecker(so); hc != nil {
		m.healthCheckers[name] = hc
		if critical {
			m.criticalHealthCheckers = append(m.criticalHealthCheckers, name)
		}
	}

	if sp := getStatsProvider(so); sp != nil {
		m.statsProviders[name] = sp
	}

	if cc := getCloseCall(so); cc != nil {
		m.closers = append(m.closers, cc)
	}
}

func (m *Manager) getCritHealthCheckers() (checkers []healthChecker, names []string) {
	m.mu.RLock()
	defer m.mu.RUnlock()

	names = append(names, m.criticalHealthCheckers...)
	for _, name := range names {
		checkers = append(checkers, m.healthCheckers[name])
	}
	return checkers, names
}

func (m *Manager) getAllHealthCheckers() (checkers []healthChecker, names []string, critical []string) {
	m.mu.RLock()
	defer m.mu.RUnlock()

	critical = append(names, m.criticalHealthCheckers...)
	checkers = make([]healthChecker, 0, len(m.healthCheckers))
	names = make([]string, 0, len(m.healthCheckers))
	for name, checker := range m.healthCheckers {
		checkers = append(checkers, checker)
		names = append(names, name)
	}
	return checkers, names, critical
}

func (m *Manager) getHealthChecker(name string) healthChecker {
	m.mu.RLock()
	defer m.mu.RUnlock()

	return m.healthCheckers[name]
}

func (m *Manager) getStatsProvider(name string) statsProvider {
	m.mu.RLock()
	defer m.mu.RUnlock()

	return m.statsProviders[name]
}

func (m *Manager) listStatsProviderNames() []string {
	m.mu.RLock()
	defer m.mu.RUnlock()

	result := make([]string, 0, len(m.statsProviders))

	for name := range m.statsProviders {
		result = append(result, name)
	}

	return result
}

type healthChecker interface {
	HealthCheck(context.Context) error
}

type statsProvider func() interface{}

type closeCall func() error
