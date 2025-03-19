package steps

func ListOfSteps(s ...DecisionStep) func() []DecisionStep {
	return func() []DecisionStep { return s }
}
