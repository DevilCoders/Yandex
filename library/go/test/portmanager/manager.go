// Package portmanager implements port allocator for arcadia.
//
// Port manager coordinates port allocation using directory
// specified in $PORT_SYNC_PATH environment variable.
//
// Protocol is compatible with Python and C++ implementation.
//
// Allocating port with ":0" works, as long as you don't release
// that port to the operating system. If you release port and later
// try to reacquire the same port, another test process may come in and
// snatch it.
package portmanager

import (
	"fmt"
	"math/rand"
	"net"
	"os"
	"path/filepath"
	"sync"
	"testing"
)

const (
	maxRetries = 1024

	portMax                = (1 << 16) - 1
	portIANAEphemeralStart = (1 << 15) + (1 << 14)
)

// New creates new PortManager that automatically releases ports back to the CI
// when test is finished.
func New(tb testing.TB) *PortManager {
	pm := &PortManager{
		tb: tb,

		portSyncDir:   os.Getenv("PORT_SYNC_PATH"),
		noRandomPorts: os.Getenv("NO_RANDOM_PORTS") != "",

		lockedPorts: make(map[int]*os.File),
	}

	var err error
	pm.rangeStart, pm.rangeEnd, err = getSafePortRange()
	if err != nil {
		tb.Fatalf("failed to determine safe port range: %v", err)
	}

	tb.Cleanup(pm.Cleanup)
	return pm
}

func getSafePortRange() (start, end int, err error) {
	explicitRange := os.Getenv("VALID_PORT_RANGE")
	if explicitRange != "" {
		_, err = fmt.Sscanf(explicitRange, "%d:%d", &start, &end)
		return
	}

	ephemeralStart, _, err := getEphemeralPortRange()
	if err != nil {
		return 0, 0, err
	}

	// Assume ephemeral range is within [1024, 65536).
	start = 1024
	end = ephemeralStart - 1

	return
}

type PortManager struct {
	tb testing.TB

	rangeStart, rangeEnd int

	portSyncDir   string
	noRandomPorts bool

	mu          sync.Mutex
	lockedPorts map[int]*os.File
}

// Cleanup releases all allocated ports back to the CI.
//
// Usually there is no need to invoke that function manually.
func (pm *PortManager) Cleanup() {
	pm.mu.Lock()
	defer pm.mu.Unlock()

	for port := range pm.lockedPorts {
		if err := pm.releasePort(port); err != nil {
			pm.tb.Fatalf("failed to release port: %v", err)
		}
	}
}

func (pm *PortManager) acquirePort(port int) error {
	if pm.portSyncDir == "" {
		return nil
	}

	lockPath := filepath.Join(pm.portSyncDir, fmt.Sprint(port))
	file, err := os.Create(lockPath)
	if err != nil {
		return err
	}

	if err := tryLockFile(file); err != nil {
		_ = file.Close()
		return err
	}

	pm.lockedPorts[port] = file
	return nil
}

func (pm *PortManager) releasePort(port int) error {
	if pm.portSyncDir == "" {
		return nil
	}

	file := pm.lockedPorts[port]
	if file == nil {
		return fmt.Errorf("port is not locked")
	}

	if err := unlockFile(file); err != nil {
		return err
	}

	if err := file.Close(); err != nil {
		return err
	}

	delete(pm.lockedPorts, port)
	return nil
}

// GetPort allocates new free port.
//
// Errors are handled by calling (testing.TB).Fatalf.
//
// Optional defaultPort is used when --no-random-ports option is passed to ya make.
func (pm *PortManager) GetPort(defaultPort ...int) int {
	port, err := pm.TryGetPort(defaultPort...)
	if err != nil {
		pm.tb.Fatal(err)
	}
	return port
}

// TryGetPort is like GetPort, but it returns error to the user instead of calling Fatalf.
func (pm *PortManager) TryGetPort(defaultPort ...int) (int, error) {
	if pm.noRandomPorts && len(defaultPort) != 0 {
		return defaultPort[0], nil
	}

	pm.mu.Lock()
	defer pm.mu.Unlock()

	var err error
	for i := 0; i < maxRetries; i++ {
		randomPort := rand.Intn(pm.rangeEnd-pm.rangeStart) + pm.rangeStart

		var lsn net.Listener
		lsn, err = net.Listen("tcp", fmt.Sprintf(":%d", randomPort))
		if err != nil {
			continue
		}
		_ = lsn.Close()

		var pkt net.PacketConn
		pkt, err = net.ListenPacket("udp", fmt.Sprintf(":%d", randomPort))
		if err != nil {
			continue
		}
		_ = pkt.Close()

		if err = pm.acquirePort(randomPort); err != nil {
			continue
		}

		return randomPort, nil
	}

	return 0, fmt.Errorf("failed to allocate free port after %d iterations: %v", maxRetries, err)
}
