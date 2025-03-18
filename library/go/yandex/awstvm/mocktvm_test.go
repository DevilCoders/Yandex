package awstvm_test

import (
	"context"
	"fmt"

	"a.yandex-team.ru/library/go/yandex/tvm"
)

type mockTvm struct {
	tvm.Client
	counters map[tvm.ClientID]int
}

func newMockTvmClient() *mockTvm {
	return &mockTvm{
		counters: make(map[tvm.ClientID]int),
	}
}

func (m *mockTvm) GetServiceTicketForID(_ context.Context, dstID tvm.ClientID) (ticket string, err error) {
	m.counters[dstID]++
	return fmt.Sprintf("ticket_for:%d;counter:%d", dstID, m.counters[dstID]), nil
}
