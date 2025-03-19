package steps

type ConditionForStepsAfterMe interface {
	Steps() []DecisionStep
	Condition() bool
}

type UnconditionalStepsContainer struct {
	children []DecisionStep
	rev      bool
}

func (ci *UnconditionalStepsContainer) Steps() []DecisionStep {
	return ci.children
}

func (ci *UnconditionalStepsContainer) Condition() bool {
	return len(ci.children) > 0
}

func NewMoreStepsIfAny(children []DecisionStep) UnconditionalStepsContainer {
	return UnconditionalStepsContainer{
		children: children,
	}
}
