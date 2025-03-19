package agent

import (
	"testing"

	"github.com/stretchr/testify/require"
)

const ContainersQueueSizeThreshold = 2

type getNumberOfNeededAutoscalingNodesInput struct {
	activeNodes                  int64
	staticNodes                  int64
	containersRunning            int64
	containersPending            int64
	containersQueueSizeThreshold int64
}

func Test_getNumberOfNeededAutoscalingNodes(t *testing.T) {
	// one autoscaling node
	input := &getNumberOfNeededAutoscalingNodesInput{
		activeNodes:                  0,
		staticNodes:                  0,
		containersRunning:            0,
		containersPending:            0,
		containersQueueSizeThreshold: ContainersQueueSizeThreshold,
	}
	numberOfNeededAutoscalingNodes := int(getNumberOfNeededAutoscalingNodes(
		input.activeNodes,
		input.staticNodes,
		input.containersRunning,
		input.containersPending,
		input.containersQueueSizeThreshold,
	))
	require.Equal(t, 0, numberOfNeededAutoscalingNodes)
}

func Test_getNumberOfNeededAutoscalingNodes2(t *testing.T) {
	// one autoscaling node
	input := &getNumberOfNeededAutoscalingNodesInput{
		activeNodes:                  2,
		staticNodes:                  1,
		containersRunning:            3,
		containersPending:            3,
		containersQueueSizeThreshold: ContainersQueueSizeThreshold,
	}
	numberOfNeededAutoscalingNodes := int(getNumberOfNeededAutoscalingNodes(
		input.activeNodes,
		input.staticNodes,
		input.containersRunning,
		input.containersPending,
		input.containersQueueSizeThreshold,
	))
	require.Equal(t, 2, numberOfNeededAutoscalingNodes)
}

func Test_getNumberOfNeededAutoscalingNodes3(t *testing.T) {
	// one autoscaling node
	input := &getNumberOfNeededAutoscalingNodesInput{
		activeNodes:                  2,
		staticNodes:                  1,
		containersRunning:            3,
		containersPending:            6,
		containersQueueSizeThreshold: ContainersQueueSizeThreshold,
	}
	numberOfNeededAutoscalingNodes := int(getNumberOfNeededAutoscalingNodes(
		input.activeNodes,
		input.staticNodes,
		input.containersRunning,
		input.containersPending,
		input.containersQueueSizeThreshold,
	))
	require.Equal(t, 3, numberOfNeededAutoscalingNodes)
}

func Test_getNumberOfNeededAutoscalingNodes4(t *testing.T) {
	// two autoscaling nodes
	input := &getNumberOfNeededAutoscalingNodesInput{
		activeNodes:                  3,
		staticNodes:                  1,
		containersRunning:            6,
		containersPending:            6,
		containersQueueSizeThreshold: ContainersQueueSizeThreshold,
	}
	numberOfNeededAutoscalingNodes := int(getNumberOfNeededAutoscalingNodes(
		input.activeNodes,
		input.staticNodes,
		input.containersRunning,
		input.containersPending,
		input.containersQueueSizeThreshold,
	))
	require.Equal(t, 4, numberOfNeededAutoscalingNodes)
}

func Test_getNumberOfNeededAutoscalingNodes5(t *testing.T) {
	// three autoscaling nodes
	input := &getNumberOfNeededAutoscalingNodesInput{
		activeNodes:                  3,
		staticNodes:                  0,
		containersRunning:            18,
		containersPending:            6,
		containersQueueSizeThreshold: ContainersQueueSizeThreshold,
	}
	numberOfNeededAutoscalingNodes := int(getNumberOfNeededAutoscalingNodes(
		input.activeNodes,
		input.staticNodes,
		input.containersRunning,
		input.containersPending,
		input.containersQueueSizeThreshold,
	))
	require.Equal(t, 4, numberOfNeededAutoscalingNodes)
}

func Test_getNumberOfNeededAutoscalingNodes6(t *testing.T) {
	// three autoscaling nodes
	input := &getNumberOfNeededAutoscalingNodesInput{
		activeNodes:                  3,
		staticNodes:                  0,
		containersRunning:            18,
		containersPending:            0,
		containersQueueSizeThreshold: ContainersQueueSizeThreshold,
	}
	numberOfNeededAutoscalingNodes := int(getNumberOfNeededAutoscalingNodes(
		input.activeNodes,
		input.staticNodes,
		input.containersRunning,
		input.containersPending,
		input.containersQueueSizeThreshold,
	))
	require.Equal(t, 0, numberOfNeededAutoscalingNodes)
}

func Test_getNumberOfNeededAutoscalingNodes7(t *testing.T) {
	// three autoscaling nodes
	input := &getNumberOfNeededAutoscalingNodesInput{
		activeNodes:                  3,
		staticNodes:                  0,
		containersRunning:            18,
		containersPending:            1,
		containersQueueSizeThreshold: ContainersQueueSizeThreshold,
	}
	numberOfNeededAutoscalingNodes := int(getNumberOfNeededAutoscalingNodes(
		input.activeNodes,
		input.staticNodes,
		input.containersRunning,
		input.containersPending,
		input.containersQueueSizeThreshold,
	))
	require.Equal(t, 3, numberOfNeededAutoscalingNodes)
}

func Test_getNumberOfNeededAutoscalingNodes8(t *testing.T) {
	// three autoscaling nodes
	input := &getNumberOfNeededAutoscalingNodesInput{
		activeNodes:                  3,
		staticNodes:                  0,
		containersRunning:            18,
		containersPending:            5,
		containersQueueSizeThreshold: ContainersQueueSizeThreshold,
	}
	numberOfNeededAutoscalingNodes := int(getNumberOfNeededAutoscalingNodes(
		input.activeNodes,
		input.staticNodes,
		input.containersRunning,
		input.containersPending,
		input.containersQueueSizeThreshold,
	))
	require.Equal(t, 4, numberOfNeededAutoscalingNodes)
}

func Test_getNumberOfNeededAutoscalingNodes9(t *testing.T) {
	// three autoscaling nodes
	input := &getNumberOfNeededAutoscalingNodesInput{
		activeNodes:                  3,
		staticNodes:                  0,
		containersRunning:            18,
		containersPending:            6,
		containersQueueSizeThreshold: ContainersQueueSizeThreshold,
	}
	numberOfNeededAutoscalingNodes := int(getNumberOfNeededAutoscalingNodes(
		input.activeNodes,
		input.staticNodes,
		input.containersRunning,
		input.containersPending,
		input.containersQueueSizeThreshold,
	))
	require.Equal(t, 4, numberOfNeededAutoscalingNodes)
}

func Test_getNumberOfNeededAutoscalingNodes10(t *testing.T) {
	// three autoscaling nodes
	input := &getNumberOfNeededAutoscalingNodesInput{
		activeNodes:                  3,
		staticNodes:                  0,
		containersRunning:            18,
		containersPending:            7,
		containersQueueSizeThreshold: ContainersQueueSizeThreshold,
	}
	numberOfNeededAutoscalingNodes := int(getNumberOfNeededAutoscalingNodes(
		input.activeNodes,
		input.staticNodes,
		input.containersRunning,
		input.containersPending,
		input.containersQueueSizeThreshold,
	))
	require.Equal(t, 5, numberOfNeededAutoscalingNodes)
}
