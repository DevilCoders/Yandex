package spinnaker

#Pipeline: {
	// Root Schema
	{
		description?: string
		// The appConfig Schema
		appConfig: {
			...
		}

		// The application Schema
		application: string

		// The id Schema
		id: string

		// The index Schema
		index: int

		// The keepWaitingPipelines Schema
		keepWaitingPipelines: bool

		// The lastModifiedBy Schema
		lastModifiedBy: string

		// The limitConcurrent Schema
		limitConcurrent: bool

		// The name Schema
		name: string

		// The parameterConfig Schema
		parameterConfig: [...{
			// The default Schema
			default: string

			// The description Schema
			description: string

			// The hasOptions Schema
			hasOptions: bool

			// The label Schema
			label: string

			// The name Schema
			name: string

			// The options Schema
			options: [...{
				// The value Schema
				value: string
				...
			}]

			// The pinned Schema
			pinned: bool

			// The required Schema
			required: bool
			...
		}]

		// The spelEvaluator Schema
		spelEvaluator: string

		// The stages Schema
		stages: [...{
			name: string,
			refId: string,
			requisiteStageRefIds: [...string],
			type: string
			...
		}]

		// The triggers Schema
		triggers: [...]
	}
}
