package spinnaker

_PipelineTraits: {
	isSaltRole:            bool
	isSvmCluster:          bool
	optionalClusterSuffix: "" | "-svm"
}

_PipelineTraitsAnyContainer: _PipelineTraits & {
	isSaltRole: false
}

_PipelineTraitsSaltRole: _PipelineTraits & {
	isSaltRole: true
}

_PipelineTraitsSVM: _PipelineTraits & {
	isSvmCluster:          true
	optionalClusterSuffix: "-svm"
}

_PipelineTraitsHW: _PipelineTraits & {
	isSvmCluster:          false
	optionalClusterSuffix: ""
}

PipelineTraitsAnyContainerHW:  _PipelineTraitsAnyContainer & _PipelineTraitsHW
PipelineTraitsAnyContainerSVM: _PipelineTraitsAnyContainer & _PipelineTraitsSVM

PipelineTraitsSaltRoleHW:  _PipelineTraitsSaltRole & _PipelineTraitsHW
PipelineTraitsSaltRoleSVM: _PipelineTraitsSaltRole & _PipelineTraitsSVM
