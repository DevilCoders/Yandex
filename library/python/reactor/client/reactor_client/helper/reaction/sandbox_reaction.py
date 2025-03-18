from enum import Enum

from .abstract_dynamic_reaction import AbstractDynamicReactionBuilder
from reactor_client import reactor_objects as r_objs


class SandboxPriorityClass(Enum):
    BACKGROUND = "BACKGROUND",
    SERVICE = "SERVICE",
    USER = "USER",


class SandboxPrioritySubclass(Enum):
    LOW = "LOW",
    NORMAL = "NORMAL",
    HIGH = "HIGH",


class SandboxReactionBuilder(AbstractDynamicReactionBuilder):
    def __init__(self):
        super(SandboxReactionBuilder, self).__init__()
        super(SandboxReactionBuilder, self).set_reaction_type("sandbox_operations", "launch_task", "2")

        self._access_secret = None
        self._task_type = None
        self._task_ttl = None
        self._ram = None
        self._priority = None
        self._acquiring_semaphores = []
        self._statuses_to_release_semaphores = []

    def set_access_secret(self, nirvana_secret_name):
        """
        :type nirvana_secret_name: str
        :rtype: SandboxReactionBuilder
        """
        self._access_secret = nirvana_secret_name
        return self

    def set_task_type(self, sandbox_task_type):
        """
        :type sandbox_task_type: str
        :rtype: SandboxReactionBuilder
        """
        self._task_type = sandbox_task_type
        return self

    def set_task_ttl(self, task_ttl_sec):
        """
        :type task_ttl_sec: int
        :rtype: SandboxReactionBuilder
        """
        self._task_ttl = task_ttl_sec
        return self

    def set_ram(self, ram_minimum_mb):
        """
        :type ram_minimum_mb: int
        :rtype: SandboxReactionBuilder
        """
        self._ram = ram_minimum_mb
        return self

    def set_priority(self, priority_class, priority_subclass):
        """
        :type priority_class: SandboxPriorityClass
        :type priority_subclass: SandboxPrioritySubclass
        :rtype: SandboxReactionBuilder
        """
        self._priority = priority_class.name + "_" + priority_subclass.name
        return self

    def add_acquiring_semaphore(self, semaphore_name, weight):
        """
        :type semaphore_name: str
        :type weight: int
        :rtype: SandboxReactionBuilder
        """
        self._acquiring_semaphores.append((semaphore_name, weight))
        return self

    def add_statuses_to_release_semaphores(self, sandbox_status_or_status_group):
        """
        :type sandbox_status_or_status_group: str (see more at https://wiki.yandex-team.ru/sandbox/tasks/statuses/)
        :rtype: SandboxReactionBuilder
        """
        self._statuses_to_release_semaphores.append(sandbox_status_or_status_group)
        return self

    @property
    def parameters_values(self):
        """
        :rtype: dict[str, r_objs.ParametersValueElement]
        """
        parameters_dict = super(SandboxReactionBuilder, self).parameters_values

        if self._access_secret is None:
            raise RuntimeError("Nirvana secret with OAuth-token to access Sandbox is not specified!")
        parameters_dict["accessSecret"] = r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=self._make_metadata_from_val(self._access_secret)))

        if self._task_type is None:
            raise RuntimeError("Sandbox task type is not specified!")
        parameters_dict["taskType"] = r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=self._make_metadata_from_val(self._task_type)))

        if self._task_ttl is not None:
            parameters_dict["taskTtl"] = r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=self._make_metadata_from_val(self._task_ttl)))
        if self._ram is not None:
            parameters_dict["ram"] = r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=self._make_metadata_from_val(self._ram)))
        if self._priority is not None:
            parameters_dict["priority"] = r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=self._make_metadata_from_val(self._priority)))
        if self._acquiring_semaphores:
            parameters_dict["acquiringSemaphores"] = r_objs.ParametersValueElement(list_=r_objs.ParametersValueList(
                [r_objs.ParametersValueElement(node=r_objs.ParametersValueNode({
                    "semaphore": r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=self._make_metadata_from_val(semaphore[0]))),
                    "weight": r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=self._make_metadata_from_val(semaphore[1]))),
                })) for semaphore in self._acquiring_semaphores]
            ))
        if self._statuses_to_release_semaphores:
            parameters_dict["releasingSemaphores"] = r_objs.ParametersValueElement(list_=r_objs.ParametersValueList(
                [r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=self._make_metadata_from_val(status))) for status in self._statuses_to_release_semaphores]
            ))
        return parameters_dict

    @property
    def inputs_values(self):
        """
        :rtype: dict[str, r_objs.InputsValueElement]
        """
        return {"customParameters": r_objs.InputsValueElement(node=r_objs.InputsValueNode(self._dict_to_input_elements_dict(self._inputs)))}
