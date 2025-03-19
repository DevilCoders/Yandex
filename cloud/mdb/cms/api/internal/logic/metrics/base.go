package metrics

import (
	"fmt"
	"sync"

	"github.com/prometheus/client_golang/prometheus"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/library/go/core/log"
)

type metrics struct {
	autodutyChangeDisk           prometheus.Gauge
	autodutyReboot               prometheus.Gauge
	autodutyTemporaryUnreachable prometheus.Gauge
	autodutyProfile              prometheus.Gauge
	autodutyPrepare              prometheus.Gauge
	autodutyDeactivate           prometheus.Gauge
	autodutyPowerOff             prometheus.Gauge
	autodutyRedeploy             prometheus.Gauge
	autodutyRepairLink           prometheus.Gauge
	autodutyTotal                prometheus.Gauge
}

var instance *Interactor
var once sync.Once

type Interactor struct {
	lg      log.Logger
	cmsdb   cmsdb.Client
	metrics metrics
}

func GetInteractor(l log.Logger, cmsdb cmsdb.Client) *Interactor {
	once.Do(func() {
		instance = &Interactor{cmsdb: cmsdb, lg: l}
		instance.initMetrics()
	})
	return instance
}

func (i *Interactor) initMetrics() {
	namespace := "autoduty"
	subsystem := "success"
	helpTemplate := "%s requests handled without human"
	i.metrics = metrics{}

	i.metrics.autodutyPrepare = prometheus.NewGauge(prometheus.GaugeOpts{
		Namespace: namespace,
		Subsystem: subsystem,
		Name:      "prepare",
		Help:      fmt.Sprintf(helpTemplate, "prepare"),
	})
	prometheus.MustRegister(i.metrics.autodutyPrepare)

	i.metrics.autodutyDeactivate = prometheus.NewGauge(prometheus.GaugeOpts{
		Namespace: namespace,
		Subsystem: subsystem,
		Name:      "deactivate",
		Help:      fmt.Sprintf(helpTemplate, "deactivate"),
	})
	prometheus.MustRegister(i.metrics.autodutyDeactivate)

	i.metrics.autodutyPowerOff = prometheus.NewGauge(prometheus.GaugeOpts{
		Namespace: namespace,
		Subsystem: subsystem,
		Name:      "power_off",
		Help:      fmt.Sprintf(helpTemplate, "power_off"),
	})
	prometheus.MustRegister(i.metrics.autodutyPowerOff)

	i.metrics.autodutyReboot = prometheus.NewGauge(prometheus.GaugeOpts{
		Namespace: namespace,
		Subsystem: subsystem,
		Name:      "reboot",
		Help:      fmt.Sprintf(helpTemplate, "reboot"),
	})
	prometheus.MustRegister(i.metrics.autodutyReboot)

	i.metrics.autodutyProfile = prometheus.NewGauge(prometheus.GaugeOpts{
		Namespace: namespace,
		Subsystem: subsystem,
		Name:      "profile",
		Help:      fmt.Sprintf(helpTemplate, "profile"),
	})
	prometheus.MustRegister(i.metrics.autodutyProfile)

	i.metrics.autodutyRedeploy = prometheus.NewGauge(prometheus.GaugeOpts{
		Namespace: namespace,
		Subsystem: subsystem,
		Name:      "redeploy",
		Help:      fmt.Sprintf(helpTemplate, "redeploy"),
	})
	prometheus.MustRegister(i.metrics.autodutyRedeploy)

	i.metrics.autodutyRepairLink = prometheus.NewGauge(prometheus.GaugeOpts{
		Namespace: namespace,
		Subsystem: subsystem,
		Name:      "repair_link",
		Help:      fmt.Sprintf(helpTemplate, "repair_link"),
	})
	prometheus.MustRegister(i.metrics.autodutyRepairLink)

	i.metrics.autodutyChangeDisk = prometheus.NewGauge(prometheus.GaugeOpts{
		Namespace: namespace,
		Subsystem: subsystem,
		Name:      "change_disk",
		Help:      fmt.Sprintf(helpTemplate, "change-disk"),
	})
	prometheus.MustRegister(i.metrics.autodutyChangeDisk)

	i.metrics.autodutyTemporaryUnreachable = prometheus.NewGauge(prometheus.GaugeOpts{
		Namespace: namespace,
		Subsystem: subsystem,
		Name:      "temporary_unreachable",
		Help:      fmt.Sprintf(helpTemplate, "temporary-unreachable"),
	})
	prometheus.MustRegister(i.metrics.autodutyTemporaryUnreachable)

	i.metrics.autodutyTotal = prometheus.NewGauge(prometheus.GaugeOpts{
		Namespace: namespace,
		Subsystem: subsystem,
		Name:      "total",
		Help:      fmt.Sprintf(helpTemplate, "total"),
	})
	prometheus.MustRegister(i.metrics.autodutyTotal)
}
