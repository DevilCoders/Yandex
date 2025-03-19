package senders

import (
	"fmt"

	"github.com/vmihailenco/msgpack"

	"a.yandex-team.ru/admins/combaine/senders/pb"
	"a.yandex-team.ru/library/go/core/log"
)

// Payload for sender.
type Payload struct {
	Tags   map[string]string
	Result interface{} // various sender payloads
}

// SenderTask ...
type SenderTask struct {
	PrevTime int64
	CurrTime int64
	Data     []*Payload
}

// RepackSenderRequest to internal representation
func RepackSenderRequest(req *pb.SenderRequest, l log.Logger) (*SenderTask, error) {
	task := SenderTask{
		PrevTime: req.PrevTime,
		CurrTime: req.CurrTime,
		Data:     make([]*Payload, 0, len(req.Data)),
	}

	// Repack sender task
	for _, d := range req.Data {
		p := Payload{Tags: d.Tags}
		if err := msgpack.Unmarshal(d.Result, &p.Result); err != nil {
			l.Errorf("Failed to unpack sender payload: %s", err)
			continue
		}
		task.Data = append(task.Data, &p)
	}
	return &task, nil
}

// GetSubgroupName helper for exstracting proper subgroup name from tags
func GetSubgroupName(tags map[string]string) (string, error) {
	subgroup, ok := tags["name"]
	if !ok {
		return "", fmt.Errorf("failed to get data tag 'name': %q", tags)
	}

	t, ok := tags["type"]
	if !ok {
		return "", fmt.Errorf("failed to get data tag 'type': %q", tags)
	}

	if t == "datacenter" {
		meta, ok := tags["metahost"]
		if !ok {
			return "", fmt.Errorf("failed to get data tag 'metahost': %q", tags)
		}
		subgroup = meta + "-" + subgroup // meta.host.name + DC1
	}
	return subgroup, nil
}

// NameStack helper type for stacking metrics name
type NameStack []string

// Push add one name to metric path
func (n *NameStack) Push(item string) {
	*n = append(*n, item)
}

// Pop remove one name from metric path
func (n *NameStack) Pop() (item string) {
	item, *n = (*n)[len(*n)-1], (*n)[:len(*n)-1]
	return item
}
