package ua

// Example {"metrics":[ {"labels" : {"name": "usefulm"},"value": 113 }]}
//https://docs.yandex-team.ru/solomon/data-collection/dataformat/json
type Metric struct {
	Value  interface{}       `json:"value"`
	Labels map[string]string `json:"labels"`
	Type   string            `json:"type,omitempty"`
}
