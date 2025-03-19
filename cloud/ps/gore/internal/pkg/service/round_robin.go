package service

type RoundRobin struct {
	PeriodDays int      `json:"period_days"`
	Services   []string `json:"services"`
}
