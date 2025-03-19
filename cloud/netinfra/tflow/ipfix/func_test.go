package ipfix

import (
	"a.yandex-team.ru/cloud/netinfra/tflow/worker"
	"bytes"
	"net"
	"testing"
	"time"
)

func TestProcessTemplateFlowSet(t *testing.T) {
	var addr IP46Addr = MakeIP46Addr(net.ParseIP("10.0.0.1"))
	var domain uint32 = 42
	data := []byte{
		0x01, 0x00, 0x00, 0x08, 0x01, 0x43, 0x00, 0x08,
		0x00, 0xe6, 0x00, 0x01, 0x00, 0x08, 0x00, 0x04,
		0x00, 0xe1, 0x00, 0x04, 0x00, 0x04, 0x00, 0x01,
		0x00, 0x07, 0x00, 0x02, 0x00, 0xe3, 0x00, 0x02,
		0x00, 0xea, 0x00, 0x04,
	}
	fset := flowset{templateFlowSetID, 40}
	var pseudoWorker *worker.Worker = nil
	wp := make(chan *worker.Worker, 1)
	//where's an "idle worker in pool"
	wp <- pseudoWorker
	err := processTemplateFlowSet(bytes.NewReader(data), addr, domain, fset, wp)
	if err != nil {
		t.Fatalf("failed to process template flow set: %v", err)
	}
	if !isTemplateKnown(addr, domain, 256) {
		t.Fatalf("failed to register template")
	}
	//worker's busy
	pseudoWorker = <-wp
	var timeout time.Duration = 1 * time.Second
	rchan := make(chan bool)
	tchan := time.After(timeout)
	go func() {
		_ = processTemplateFlowSet(bytes.NewReader(data), addr, domain, fset, wp)
		rchan <- true

	}()
	select {
	case <-tchan:
		t.Errorf("process hung on processing same template")
		return
	case <-rchan:
		break
	}
	data[1] = 1
	tchan = time.After(timeout)
	go func() {
		_ = processTemplateFlowSet(bytes.NewReader(data), addr, domain, fset, wp)
		rchan <- true
	}()
	select {
	case <-rchan:
		t.Errorf("processed new template while workers ain't idle")
		return
	case <-tchan:
		break
	}
	tchan = time.After(timeout)
	wp <- pseudoWorker
	select {
	case <-tchan:
		t.Errorf("have not processed new template")
		return
	case <-rchan:
		if !isTemplateKnown(addr, domain, 257) {
			t.Errorf("failed to register new template")
			return
		}
	}
}
