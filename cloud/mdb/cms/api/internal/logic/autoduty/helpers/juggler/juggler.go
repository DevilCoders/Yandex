package juggler

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/library/go/slices"
)

//go:generate ../../../../../../../scripts/mockgen.sh JugglerChecker

type FQDNGroupByJugglerCheck struct {
	OK    []string
	NotOK []string
}

type JugglerService string

const Unreachable string = "UNREACHABLE"

func (r FQDNGroupByJugglerCheck) IsOK(fqdn string) bool {
	return slices.ContainsString(r.OK, fqdn)
}

type JugglerChecker interface {
	Check(ctx context.Context, fqdns []string, services []string, now time.Time) (FQDNGroupByJugglerCheck, error)
}

type JugglerServiceChecker struct {
	jgrl      juggler.API
	threshold time.Duration
}

func isFQDNByServiceOK(events []juggler.RawEvent, now time.Time, fqdn string, service string, threshold time.Duration) bool {
	var considerableEvents []juggler.RawEvent
	for _, event := range events {
		if event.ReceivedTime.Before(now.Add(-threshold)) || event.Host != fqdn || event.Service != service {
			continue
		}
		considerableEvents = append(considerableEvents, event)
	}

	isOK := true
	countOk := 0
	countCrit := 0
	if len(considerableEvents) == 0 {
		isOK = false
	} else {
		for _, event := range considerableEvents {
			if event.Status != "OK" {
				countCrit += 1
			} else {
				countOk += 1
			}
		}
		if countCrit >= countOk {
			isOK = false
		}
	}
	return isOK
}

func (rc *JugglerServiceChecker) Check(ctx context.Context, fqdns []string, services []string, now time.Time) (FQDNGroupByJugglerCheck, error) {
	result := FQDNGroupByJugglerCheck{}
	jEvents, err := rc.jgrl.RawEvents(ctx, fqdns, services)
	if err != nil {
		return result, err
	}
	for _, fqdn := range fqdns {
		for _, serviceName := range services {
			if isFQDNByServiceOK(jEvents, now, fqdn, serviceName, rc.threshold) {
				result.OK = append(result.OK, fqdn)
			} else {
				result.NotOK = append(result.NotOK, fqdn)
			}
		}

	}

	return result, nil
}

func NewJugglerReachabilityChecker(
	jgrl juggler.API,
	windowMin int,
) JugglerChecker {
	return NewCustomJugglerChecker(jgrl, time.Duration(windowMin)*time.Minute)
}

func NewCustomJugglerChecker(
	jgrl juggler.API,
	threshold time.Duration,
) JugglerChecker {
	return &JugglerServiceChecker{
		jgrl:      jgrl,
		threshold: threshold,
	}
}
