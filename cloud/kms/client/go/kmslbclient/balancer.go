package kmslbclient

import (
	"bytes"
	"context"
	"errors"
	"math"
	"math/rand"
	"runtime/debug"
	"sort"
	"sync"
	"time"

	"go.uber.org/atomic"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/connectivity"
	"google.golang.org/grpc/health/grpc_health_v1"
	"google.golang.org/grpc/keepalive"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/library/go/core/log"
)

const (
	healthService     = "grpc.health.v1.Health"
	healthCheckMethod = "/" + healthService + "/Check"
)

type endpoint struct {
	name string
	addr string

	pingTime time.Duration
	// newPingTime can be accessed concurrently from multiple goroutines.
	newPingTime atomic.Duration

	conns []*grpc.ClientConn

	dead        atomic.Bool
	removeAfter atomicTime

	fails *fixedTimeWindow

	curRequests atomic.Int32
}

func (e *endpoint) String() string {
	return e.name
}

type endpointGroup struct {
	endpoints []*endpoint
}

func (e endpointGroup) String() string {
	if len(e.endpoints) == 0 {
		return "[]"
	}
	buf := bytes.NewBufferString("[")
	buf.WriteString(e.endpoints[0].name)
	for _, ep := range e.endpoints[1:] {
		buf.WriteString(" ")
		buf.WriteString(ep.name)
	}
	buf.WriteString("]")
	return buf.String()
}

type pingingBalancer struct {
	options     BalancerOptions
	dialOptions []grpc.DialOption

	resolver Resolver

	logger log.Logger

	endpoints map[string]*endpoint

	delayedCloseEndpoints []*endpoint

	balancedGroups     []endpointGroup
	balancedGroupsLock sync.RWMutex

	closeC chan struct{}
	closed bool
}

func reconnectAndRebalanceEndpoints(b *pingingBalancer) {
	defer func() {
		if r := recover(); r != nil {
			b.logger.Errorf("Recovering from panic in reconnectAndRebalanceEndpoints: %v\nStacktrace: %s", r, string(debug.Stack()))
		}
	}()

	finishDelayedClose(b)
	removeDeadEndpoints(b)
	createMissingEndpoints(b)
	rebalanceEndpoints(b)
}

func finishDelayedClose(b *pingingBalancer) {
	for _, ep := range b.delayedCloseEndpoints {
		b.logger.Warnf("Closing dead endpoint %v", ep)
		closeEndpoint(b, ep)
	}
	b.delayedCloseEndpoints = nil
}

func removeDeadEndpoints(b *pingingBalancer) {
	now := time.Now()
	for addr, ep := range b.endpoints {
		if ep.dead.Load() && ep.removeAfter.Load().Before(now) {
			b.logger.Warnf("Removing dead endpoint %v from the list", ep)
			delete(b.endpoints, addr)
			// Do not immediately close endpoint: this is useful for cases when the panic mode is enabled
			// for some reason (e.g. we have only one endpoint and it became dead due to a network flip).
			// Allow the new endpoint be created to replace the dead one and close the dead only after that.
			b.delayedCloseEndpoints = append(b.delayedCloseEndpoints, ep)
		}
	}
}

func createMissingEndpoints(b *pingingBalancer) {
	callCtx, cancelFunc := context.WithTimeout(context.Background(), b.options.MaxTimePerTry)
	defer cancelFunc()
	resolvedAddrs, err := b.resolver.GetEndpoints(callCtx)
	if err != nil {
		return
	}

	var toCreate []ResolvedAddr
	for _, addr := range resolvedAddrs {
		_, ok := b.endpoints[addr.addr]
		if !ok {
			toCreate = append(toCreate, addr)
		}
	}
	b.logger.Infof("Got %d endpoints from resolver: %v, need to create %d: %v\n", len(resolvedAddrs), resolvedAddrs, len(toCreate), toCreate)
	if len(toCreate) > 0 {
		startTime := time.Now()
		created := make(chan *endpoint)
		for _, addr := range toCreate {
			b.logger.Infof("Creating endpoint for resolved address %v", addr)
			go func(addr ResolvedAddr) {
				defer func() {
					if r := recover(); r != nil {
						b.logger.Errorf("Recovering from panic in makeAndDialEndpoint for %v, error is: %v\nStacktrace: %s", addr, r, string(debug.Stack()))
						created <- nil
					}
				}()
				created <- makeAndDialEndpoint(b, addr)
			}(addr)
		}
		createdCount := 0
		for i := 0; i < len(toCreate); i++ {
			ep := <-created
			if ep != nil {
				b.endpoints[ep.addr] = ep
				createdCount++
			}
		}
		b.logger.Infof("Created %d new endpoints in %v", createdCount, time.Since(startTime))
	}

	// I can't believe that Go still does not have a set container.
	resolvedAddrsSet := map[string]bool{}
	for _, addr := range resolvedAddrs {
		resolvedAddrsSet[addr.addr] = true
	}
	for addr, ep := range b.endpoints {
		if _, ok := resolvedAddrsSet[addr]; !ok {
			b.logger.Infof("Removing endpoint %v, not included in resolved addrs", addr)
			closeEndpoint(b, ep)
			delete(b.endpoints, addr)
		}
	}
}

func makeEndpointInterceptor(b *pingingBalancer, ep *endpoint) grpc.DialOption {
	f := func(b *pingingBalancer, ep *endpoint, ctx context.Context, method string, req, reply interface{}, cc *grpc.ClientConn, invoker grpc.UnaryInvoker, opts ...grpc.CallOption) error {
		// Do nothing for pings.
		if method == healthCheckMethod {
			return invoker(ctx, method, req, reply, cc, opts...)
		}

		// Add some slack to the timeout. This does not override the deadline of the parent ctx, so we canhit Deadline
		// earlier than MaxTimePerTry anyway.
		timeWithSlack := b.options.MaxTimePerTry + b.options.MaxTimePerTry/4
		callCtx, cancelFunc := context.WithTimeout(ctx, timeWithSlack)
		defer cancelFunc()

		// There is a little race condition here between time-of-check and time-of-use: curRequests is checked in NextConn()
		// while it is incremented here. This can lead to node overload when too many calls are started simulataneously.
		ep.curRequests.Add(1)
		startTime := time.Now()
		err := invoker(callCtx, method, req, reply, cc, opts...)
		callTime := time.Since(startTime)
		ep.curRequests.Sub(1)

		code := status.Code(err)
		// We do not consider codes.Deadline to be a condition for incrementing the fail count, because there could
		// be less time until the ctx.Deadline than MaxTimePerTry
		if code == codes.Unavailable || code == codes.ResourceExhausted || callTime > b.options.MaxTimePerTry {
			desc := status.Convert(err).Message()
			b.logger.Infof("Adding fail (code %d, call time %s, desc '%s') for endpoint %v", code, callTime, desc, ep)
			markFailAndCheckDead(b, ep)
		}
		return err
	}

	return grpc.WithUnaryInterceptor(func(ctx context.Context, method string, req, reply interface{}, cc *grpc.ClientConn, invoker grpc.UnaryInvoker, opts ...grpc.CallOption) error {
		return f(b, ep, ctx, method, req, reply, cc, invoker, opts...)
	})
}

func markFailAndCheckDead(b *pingingBalancer, ep *endpoint) {
	failWindow, isFull := ep.fails.add(time.Now())
	if isFull && failWindow < b.options.FailWindowPerHost {
		markDead(b, ep)
	}
}

func markDead(b *pingingBalancer, ep *endpoint) {
	b.logger.Warnf("Fail count hit for endpoint %v, marking as dead", ep)
	// The ordering is important: it is reverse compared to the checks in removeDeadEndpoints.
	if !ep.dead.Load() {
		ep.removeAfter.Store(time.Now().Add(b.options.RetryHostAfter))
		ep.dead.Store(true)
	}
}

func makeAndDialEndpoint(b *pingingBalancer, addr ResolvedAddr) *endpoint {
	ep := &endpoint{
		name:  addr.name,
		addr:  addr.addr,
		fails: newFixedTimeWindow(b.options.MaxFailCountPerHost),
	}
	var grpcOptions []grpc.DialOption
	grpcOptions = append(grpcOptions, b.dialOptions...)
	keepaliveOption := grpc.WithKeepaliveParams(keepalive.ClientParameters{
		// Use HTTP2 pings with half keepalive time so that connections are guaranteed checked between
		// two keep-alive checks and rebalances.
		Time:                b.options.KeepAliveTime / 2,
		Timeout:             b.options.MaxTimePerTry,
		PermitWithoutStream: false,
	})
	//nolint:SA1019
	backoffOption := grpc.WithBackoffConfig(grpc.BackoffConfig{
		// Make sure we do not wait for too long when trying to reconnect to the node.
		MaxDelay: b.options.RetryHostAfter,
	})
	grpcOptions = append(grpcOptions, keepaliveOption, backoffOption, makeEndpointInterceptor(b, ep))
	var conns []*grpc.ClientConn
	for i := 0; i < b.options.ConnectionsPerHost; i++ {
		conn, err := grpc.Dial(addr.addr, grpcOptions...)
		if err != nil {
			b.logger.Warnf("Could not dial %s: %v", addr, err)
			// Close the already open connections.
			for _, c := range conns {
				_ = c.Close()
			}
			return nil
		}
		conns = append(conns, conn)
	}
	ep.conns = conns

	err := measureAndUpdatePing(b, ep)

	if err != nil {
		// Immediately close all connections, do not put endpoint in delayed closing list.
		for _, conn := range ep.conns {
			_ = conn.Close()
		}
		return nil
	}
	return ep
}

func closeEndpoint(b *pingingBalancer, ep *endpoint) {
	for _, conn := range ep.conns {
		err := conn.Close()
		if err != nil {
			b.logger.Infof("Error when closing connection to endpoint %v", ep)
		}
	}
}

func rebalanceEndpoints(b *pingingBalancer) {
	var endpoints []*endpoint
	for _, ep := range b.endpoints {
		endpoints = append(endpoints, ep)
	}
	for _, ep := range endpoints {
		ep.pingTime = ep.newPingTime.Load()
	}

	balancedGroups := splitEndpointsByPing(b, endpoints)

	b.balancedGroupsLock.Lock()
	b.balancedGroups = balancedGroups
	b.balancedGroupsLock.Unlock()
}

func splitEndpointsByPing(b *pingingBalancer, endpoints []*endpoint) []endpointGroup {
	sort.Slice(endpoints, func(i, j int) bool {
		return endpoints[i].pingTime < endpoints[j].pingTime
	})
	b.logger.Debugf("Sorted endpoints by ping: %v", endpoints)

	var ret []endpointGroup
	var curGroup endpointGroup
	var prevPing time.Duration
	for i, ep := range endpoints {
		if !similarPings(b, ep.pingTime, prevPing) {
			if i > 0 {
				ret = append(ret, curGroup)
				curGroup = endpointGroup{}
			}
		}
		curGroup.endpoints = append(curGroup.endpoints, ep)
		prevPing = ep.pingTime
	}
	if len(curGroup.endpoints) > 0 {
		ret = append(ret, curGroup)
	}
	b.logger.Debugf("Got endpoint groups: %+v", ret)

	return ret
}

func similarPings(b *pingingBalancer, nextPing time.Duration, prevPing time.Duration) bool {
	if nextPing-prevPing < b.options.SimilarPingThreshold {
		return true
	}
	if prevPing > 0 && float64(nextPing)/float64(prevPing) < b.options.SimilarPingRatio {
		return true
	}
	return false
}

func updateEndpointPingsAndRebalance(b *pingingBalancer) {
	defer func() {
		if r := recover(); r != nil {
			b.logger.Errorf("Recovering from panic in updateEndpointPingsAndRebalance: %v\nStacktrace: %s", r, string(debug.Stack()))
		}
	}()

	updateEndpointPings(b)
	rebalanceEndpoints(b)
}

func updateEndpointPings(b *pingingBalancer) {
	startTime := time.Now()
	b.logger.Debugf("Started pinging endpoints")
	wg := sync.WaitGroup{}
	wg.Add(len(b.endpoints))
	for _, ep := range b.endpoints {
		go func(ep *endpoint) {
			defer func() {
				if r := recover(); r != nil {
					b.logger.Errorf("Recovering from panic in measureAndUpdatePing for %v, error is: %v \n", ep, r)
					wg.Done()
				}
			}()

			// Sleep before measuring the ping to prevent potential client overload.
			if b.options.Jitter > 0.0 {
				jitterMult := b.options.Jitter * rand.Float64()
				time.Sleep(time.Duration(float64(b.options.PingRefreshTime) * jitterMult))
			}

			err := measureAndUpdatePing(b, ep)
			if err != nil {
				// Do not immediately remove endpoint from the list of endpoints (it may still be used in panic mode).
				markDead(b, ep)
			}
			wg.Done()
		}(ep)
	}
	wg.Wait()
	b.logger.Debugf("Pinged endpoints in %v", time.Since(startTime))
}

func measureAndUpdatePing(b *pingingBalancer, ep *endpoint) error {
	times := make([]time.Duration, b.options.NumPings)
	numErrors := 0
	for i := 0; i < b.options.NumPings; i++ {
		t, err := pingConn(ep.conns[i%len(ep.conns)], b.options.MaxPing)
		times[i] = t
		if err != nil {
			b.logger.Warnf("Error when pinging %v: %v", ep, err)
			numErrors++
			if numErrors >= b.options.MaxFailCountPerHost {
				break
			}
			continue
		}
	}
	if numErrors >= b.options.MaxFailCountPerHost || numErrors == b.options.NumPings {
		b.logger.Warnf("Fail limit hit when pinging %v, marking as dead", ep)
		return errors.New("no ping")
	}

	if len(times) == 0 {
		b.logger.Debugf("Got no pings for endpoint %v", ep)
		return nil
	}

	// Minimum ping time is a more stable measurement than other percentiles.
	minTime := times[0]
	for i := 1; i < len(times); i++ {
		if times[i] < minTime {
			minTime = times[i]
		}
	}
	b.logger.Debugf("Pings for endpoint %v: min: %v, times %v", ep, minTime, times)
	ep.newPingTime.Store(minTime)
	return nil
}

func pingConn(conn *grpc.ClientConn, maxTime time.Duration) (time.Duration, error) {
	client := grpc_health_v1.NewHealthClient(conn)
	ctx, cancelFunc := context.WithTimeout(context.Background(), maxTime)
	defer cancelFunc()
	req := grpc_health_v1.HealthCheckRequest{
		Service: healthService,
	}
	startTime := time.Now()
	resp, err := client.Check(ctx, &req)
	// Ignore errors if the service is invalid.
	st := status.Code(err)
	if st != codes.OK && st != codes.NotFound && st != codes.InvalidArgument {
		return math.MaxInt64, err
	}
	// If the service exists and reports to be not SERVING, consider the endpoint dead.
	if st == codes.OK && resp.GetStatus() != grpc_health_v1.HealthCheckResponse_SERVING {
		return math.MaxInt64, errors.New("client responded with status " + resp.GetStatus().String())
	}
	return time.Since(startTime), nil
}

func closeConns(b *pingingBalancer) {
	for _, ep := range b.endpoints {
		closeEndpoint(b, ep)
	}
	for _, ep := range b.delayedCloseEndpoints {
		closeEndpoint(b, ep)
	}
}

func runPingingBalancer(b *pingingBalancer) {
	keepAliveTicker := time.NewTicker(b.options.KeepAliveTime)
	pingTicker := time.NewTicker(b.options.PingRefreshTime)
	for {
		done := false
		select {
		case <-keepAliveTicker.C:
			reconnectAndRebalanceEndpoints(b)
		case <-pingTicker.C:
			updateEndpointPingsAndRebalance(b)
		case <-b.closeC:
			done = true
		}
		if done {
			break
		}
	}
	keepAliveTicker.Stop()
	pingTicker.Stop()
	closeConns(b)
}

type pingingCallState struct {
	b           *pingingBalancer
	endpoints   []*endpoint
	endpointIdx int
	panicMode   bool
}

func (s *pingingCallState) NextConn() (*grpc.ClientConn, error) {
	allDead := true
	for {
		if s.endpointIdx >= len(s.endpoints) {
			if s.panicMode || !s.b.options.AllowPanicMode {
				if allDead {
					return nil, status.Error(codes.Unavailable, "all connections are dead")
				} else {
					return nil, status.Error(codes.ResourceExhausted, "backends are overloaded: max requests hit")
				}
			} else {
				// Retry the whole iteration in panic mode (with panicMode set to true).
				s.panicMode = true
				s.endpointIdx = 0
			}
		}

		ep := s.endpoints[s.endpointIdx]
		s.endpointIdx++
		if ep.dead.Load() && !s.panicMode {
			continue
		}
		conn := getConnForEndpoint(ep, s.panicMode)
		if conn == nil {
			markFailAndCheckDead(s.b, ep)
			continue
		}
		allDead = false
		if hitMaxRequests(s, ep) {
			continue
		}
		return conn, nil
	}
}

func hitMaxRequests(s *pingingCallState, ep *endpoint) bool {
	return s.b.options.MaxConcurrentRequestsPerHost > 0 && ep.curRequests.Load() > int32(s.b.options.MaxConcurrentRequestsPerHost)
}

func getConnForEndpoint(ep *endpoint, panicMode bool) *grpc.ClientConn {
	startFrom := rand.Intn(len(ep.conns))
	numConns := len(ep.conns)
	for i := 0; i < numConns; i++ {
		conn := ep.conns[(startFrom+i)%numConns]
		if conn.GetState() == connectivity.Ready {
			return conn
		}
	}

	if panicMode {
		// We are in panic mode, return any connection.
		return ep.conns[startFrom]
	}
	// Try to find a non-failed connection if none are ready.
	for i := 0; i < numConns; i++ {
		conn := ep.conns[(startFrom+i)%numConns]
		if conn.GetState() != connectivity.TransientFailure && conn.GetState() != connectivity.Shutdown {
			return conn
		}
	}
	return nil
}

func (s *pingingCallState) Close() {
}

func (b *pingingBalancer) NewCall() (CallState, error) {
	b.balancedGroupsLock.RLock()
	groups := b.balancedGroups
	b.balancedGroupsLock.RUnlock()

	if len(groups) == 0 {
		return nil, status.Error(codes.Unavailable, "unable to resolve hosts")
	}
	state := &pingingCallState{
		b:           b,
		endpoints:   shuffleEndpoints(groups),
		endpointIdx: 0,
		panicMode:   false,
	}
	return state, nil
}

func shuffleEndpoints(groups []endpointGroup) []*endpoint {
	numEndpoints := 0
	for _, group := range groups {
		numEndpoints += len(group.endpoints)
	}
	ret := make([]*endpoint, numEndpoints)
	pos := 0
	for _, group := range groups {
		groupLen := len(group.endpoints)
		copy(ret[pos:], group.endpoints)
		rand.Shuffle(groupLen, func(i int, j int) {
			ret[pos+i], ret[pos+j] = ret[pos+j], ret[pos+i]
		})
		pos += groupLen
	}
	return ret
}

func (b *pingingBalancer) Close() {
	if !b.closed {
		close(b.closeC)
		b.closed = true
	}
}

type CallState interface {
	NextConn() (*grpc.ClientConn, error)
	Close()
}

type Balancer interface {
	NewCall() (CallState, error)
	Close()
}

func NewPingingBalancer(options BalancerOptions, dialOptions []grpc.DialOption, resolver Resolver, logger log.Logger) Balancer {
	b := &pingingBalancer{
		options:     options,
		dialOptions: dialOptions,
		resolver:    resolver,
		logger:      logger,
		endpoints:   map[string]*endpoint{},
		closeC:      make(chan struct{}),
	}
	reconnectAndRebalanceEndpoints(b)
	go runPingingBalancer(b)
	return b
}
