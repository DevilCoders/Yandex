# coding=utf-8
from .base import TestBase, fix_workflow_description
from nirvana_api.workflow import WorkflowInstance

import json

import nirvana_api.json_rpc as json_rpc


class TestWorkflowInstance(TestBase):
    def setup(self):
        instance_id = self.api.clone_workflow_instance(workflow_id=self.workflow.id)
        self.workflow_instance = WorkflowInstance(self.api, instance_id)

    def test_create_workflow_instance(self):
        description = json.dumps(fix_workflow_description(self.workflow_instance.get()), sort_keys=True, cls=json_rpc.DefaultEncoder)
        assert self.canonical_desc == description

    def tearDown(self):
        self.api.delete_workflow_instance(self.workflow_instance.id)
