import enum
from datetime import datetime, timedelta

import pytz
from sqlalchemy import (
    Boolean,
    Column,
    DateTime,
    Enum,
    ForeignKey,
    Integer,
    JSON,
    LargeBinary,
    String,
    Text,
    UniqueConstraint,
)
from sqlalchemy.orm import relationship

from . import Base


class ClusterRecipe(Base):
    __tablename__ = "cluster_recipes"

    id = Column(Integer, primary_key=True)
    manually_created = Column(Boolean, default=False)
    arcadia_path = Column(String(200), nullable=False)
    name = Column(String(100), nullable=False)
    description = Column(Text, nullable=False)

    clusters = relationship("Cluster", back_populates="recipe")
    files = relationship("ClusterRecipeFile", back_populates="recipe", cascade="all, delete-orphan")

    def __repr__(self):
        return f"ClusterRecipe(name={self.name}, path={self.arcadia_path})"


class ClusterRecipeFile(Base):
    __tablename__ = "cluster_recipe_files"
    __table_args__ = (
        UniqueConstraint("recipe_id", "relative_file_path", name="recipe_and_file_path"),
    )

    id = Column(Integer, primary_key=True)

    recipe_id = Column(Integer, ForeignKey(ClusterRecipe.id))
    recipe = relationship(ClusterRecipe, back_populates="files", single_parent=True)

    relative_file_path = Column(String(2000), nullable=False)

    content = Column(LargeBinary(), nullable=True)


class Cluster(Base):
    __tablename__ = "clusters"

    id = Column(Integer, primary_key=True)
    name = Column(String(100), nullable=False, default="", server_default="")
    slug = Column(String(100), nullable=False, default="", server_default="")

    recipe_id = Column(Integer, ForeignKey(ClusterRecipe.id))
    recipe = relationship(ClusterRecipe, back_populates="clusters")

    manually_created = Column(Boolean, default=False)
    arcadia_path = Column(String(200), nullable=False, server_default="")

    last_deploy_attempt_finish_time = Column(DateTime(timezone=True), nullable=True)

    variables = relationship("ClusterVariable", back_populates="cluster", cascade="all, delete-orphan")

    configs = relationship("ProberConfig", back_populates="cluster", cascade="all, delete-orphan")

    deployments = relationship("ClusterDeployment", back_populates="cluster", cascade="all, delete-orphan")

    deploy_policy = relationship(
        "ClusterDeployPolicy", back_populates="cluster", uselist=False, cascade="all, delete-orphan"
    )

    def __repr__(self):
        return f"Cluster(id={self.id}, name={self.name}, slug={self.slug}, " \
               f"recipe={self.recipe}, variables={self.variables})"

    def get_variable_value(self, variable_name: str, default_value: any = None):
        for variable in self.variables:
            if variable.name == variable_name:
                return variable.value
        if default_value is not None:
            return default_value
        raise KeyError(f"Variable {variable_name} not found in cluster {self}")

    def is_ready_for_deploy(self) -> bool:
        if self.deploy_policy is None:
            return True

        if self.deploy_policy.type == ClusterDeployPolicyType.MANUAL:
            return self.deploy_policy.ship

        if self.deploy_policy.type == ClusterDeployPolicyType.REGULAR:
            return self.last_deploy_attempt_finish_time.replace(tzinfo=pytz.UTC) + \
                   timedelta(seconds=self.deploy_policy.sleep_interval) < datetime.now(pytz.utc)

        return False


class ClusterVariable(Base):
    __tablename__ = "cluster_variables"
    __table_args__ = (
        UniqueConstraint("cluster_id", "name", name="cluster_and_name"),
    )

    id = Column(Integer, primary_key=True)

    cluster_id = Column(Integer, ForeignKey(Cluster.id))
    cluster = relationship(Cluster, back_populates="variables", single_parent=True)

    name = Column(String(100), nullable=False)
    value = Column(JSON, nullable=False)

    def __repr__(self):
        return f"<{self.name}={self.value!r}>"


class ClusterDeployPolicyType(str, enum.Enum):
    MANUAL = "MANUAL"
    REGULAR = "REGULAR"

    def __repr__(self):
        return repr(self.value)


class ClusterDeployPolicy(Base):
    __tablename__ = "cluster_deploy_policies"

    id = Column(Integer, primary_key=True)

    parallelism = Column(Integer, default=10, nullable=False)
    plan_timeout = Column(Integer, nullable=True)
    apply_timeout = Column(Integer, nullable=True)

    cluster_id = Column(Integer, ForeignKey(Cluster.id), unique=True)
    cluster = relationship(Cluster, back_populates="deploy_policy", single_parent=True)

    type = Column(Enum(ClusterDeployPolicyType))

    __mapper_args__ = {
        'with_polymorphic': '*',
        "polymorphic_on": type,
        'polymorphic_identity': 'UNDEFINED',
    }

    def __repr__(self):
        return f"ClusterDeployPolicy(id={self.id}, cluster={self.cluster}, " + \
               f"parallelism={self.parallelism}, plan_timeout={self.plan_timeout}, apply_timeout={self.apply_timeout})"


class ManualClusterDeployPolicy(ClusterDeployPolicy):
    ship = Column(Boolean, default=False)

    __mapper_args__ = {
        'polymorphic_identity': ClusterDeployPolicyType.MANUAL,
        'polymorphic_load': 'inline'
    }


class RegularClusterDeployPolicy(ClusterDeployPolicy):
    sleep_interval = Column(Integer, default=60)

    __mapper_args__ = {
        'polymorphic_identity': ClusterDeployPolicyType.REGULAR,
        'polymorphic_load': 'inline'
    }


class Prober(Base):
    __tablename__ = "probers"

    id = Column(Integer, primary_key=True)

    manually_created = Column(Boolean, default=False)
    arcadia_path = Column(String(200), nullable=False)
    name = Column(String(100), nullable=False)
    slug = Column(String(100), nullable=False, default="", server_default="")
    description = Column(Text, nullable=False)
    runner = relationship("ProberRunner", back_populates="prober", uselist=False)

    configs = relationship("ProberConfig", back_populates="prober", cascade="all, delete-orphan")
    files = relationship("ProberFile", back_populates="prober", cascade="all, delete-orphan")

    def __repr__(self):
        return f"Prober(id={self.id}, name={self.name}, slug={self.slug}, runner={self.runner})"


class ProberRunnerType(str, enum.Enum):
    BASH = "BASH"

    def __repr__(self):
        return repr(self.value)


class ProberRunner(Base):
    __tablename__ = "prober_runners"

    id = Column(Integer, primary_key=True)
    type = Column(Enum(ProberRunnerType))

    prober_id = Column(Integer, ForeignKey("probers.id"))
    prober = relationship("Prober", back_populates="runner", single_parent=True)

    __mapper_args__ = {
        "polymorphic_on": type,
    }


class BashProberRunner(ProberRunner):
    command = Column(Text, nullable=False)

    __mapper_args__ = {
        "polymorphic_identity": ProberRunnerType.BASH,
    }

    def __repr__(self):
        return f"Bash({self.command})"


class UploadProberLogPolicy(str, enum.Enum):
    NONE = "NONE"
    FAIL = "FAIL"
    ALL = "ALL"

    def __repr__(self):
        return repr(self.value)


class ProberConfig(Base):
    __tablename__ = "prober_configs"

    __table_args__ = (
        UniqueConstraint("cluster_id", "hosts_re", name="cluster_id_and_hosts_re"),
    )

    id = Column(Integer, primary_key=True)

    prober_id = Column(Integer, ForeignKey(Prober.id))
    prober = relationship(Prober, back_populates="configs", single_parent=True)

    manually_created = Column(Boolean, default=False)

    # Optional cluster_id
    cluster_id = Column(Integer, ForeignKey(Cluster.id), nullable=True, default=None)
    cluster = relationship(Cluster, back_populates="configs", single_parent=True)

    # Optional hosts regular expression, i.e. '^agent\d+', can be specified together with cluster_id or separately
    hosts_re = Column(String(200), nullable=True, default=None)

    # Actually, true prober parameters are below. NULL means that value is inherited from more global config.
    is_prober_enabled = Column(Boolean, nullable=True)
    interval_seconds = Column(Integer, nullable=True)
    timeout_seconds = Column(Integer, nullable=True)
    s3_logs_policy = Column(Enum(UploadProberLogPolicy), nullable=True)
    default_routing_interface = Column(String(50), nullable=True)
    dns_resolving_interface = Column(String(50), nullable=True)

    matrix_variables = relationship("ProberMatrixVariable", back_populates="prober_config", cascade="all, delete-orphan")
    variables = relationship("ProberVariable", back_populates="prober_config", cascade="all, delete-orphan")

    def __repr__(self):
        return f"ProberConfig(id={self.id}, prober={self.prober}, " \
               f"cluster={self.cluster if self.cluster_id else '×'}, " \
               f"hosts_re={self.hosts_re if self.hosts_re else '×'}), " \
               f"s3_logs_policy={self.s3_logs_policy}, " \
               f"variables={self.variables if self.variables else '×'})"


class ProberFile(Base):
    __tablename__ = "prober_files"
    __table_args__ = (
        UniqueConstraint("prober_id", "relative_file_path", name="prober_and_file_path"),
    )

    id = Column(Integer, primary_key=True)

    prober_id = Column(Integer, ForeignKey(Prober.id))
    prober = relationship(Prober, back_populates="files", single_parent=True)

    relative_file_path = Column(String(2000), nullable=False)
    md5_hexdigest = Column(String(32), nullable=True)
    content = Column(LargeBinary(), nullable=True)

    is_executable = Column(Boolean, default=False, server_default="false", nullable=False)

    def __repr__(self):
        return f"ProberFile(id={self.id}, prober={self.prober}, " \
               f"relative_file_path={self.relative_file_path}, " \
               f"md5_hexdigest={self.md5_hexdigest})"


class ProberMatrixVariable(Base):
    __tablename__ = "prober_matrix_variables"
    __table_args__ = (
        UniqueConstraint("prober_config_id", "name", name="prober_config_id_and_matrix_variable_name"),
    )

    id = Column(Integer, primary_key=True)

    prober_config_id = Column(ForeignKey(ProberConfig.id))
    prober_config = relationship(ProberConfig, back_populates="matrix_variables", uselist=False, single_parent=True)

    name = Column(String(128), nullable=False)
    values = Column(JSON, nullable=False)

    def __repr__(self):
        return f"{self.__class__.__name__}(id={self.id}, name={self.name}, values={self.values!r})"


class ProberVariable(Base):
    __tablename__ = "prober_variables"
    __table_args__ = (
        UniqueConstraint("prober_config_id", "name", name="prober_config_id_and_variable_name"),
    )

    id = Column(Integer, primary_key=True)

    prober_config_id = Column(ForeignKey(ProberConfig.id))
    prober_config = relationship(ProberConfig, back_populates="variables", uselist=False, single_parent=True)

    name = Column(String(128), nullable=False)
    value = Column(JSON, nullable=False)

    def __repr__(self):
        return f"{self.__class__.__name__}(id={self.id}, name={self.name}, value={self.value!r})"


class ClusterDeploymentStatus(str, enum.Enum):
    RUNNING = "RUNNING"
    COMPLETED = "COMPLETED"
    # We want to distinguish deployments with empty plan without any further action and true deployments.
    COMPLETED_WITH_EMPTY_PLAN = "COMPLETED_WITH_EMPTY_PLAN"
    INIT_FAILED = "INIT_FAILED"
    PLAN_FAILED = "PLAN_FAILED"
    APPLY_FAILED = "APPLY_FAILED"

    def __repr__(self):
        return repr(self.value)


class ClusterDeployment(Base):
    __tablename__ = "cluster_deployments"

    id = Column(Integer, primary_key=True)
    status = Column(Enum(ClusterDeploymentStatus))
    start = Column(DateTime(timezone=True), nullable=False)
    finish = Column(DateTime(timezone=True), nullable=True)

    cluster_id = Column(Integer, ForeignKey(Cluster.id))
    cluster = relationship(Cluster, back_populates="deployments", single_parent=True)

    def __repr__(self):
        return f"ClusterDeployment(status={self.status!r}, " \
               f"cluster={self.cluster.slug}, " \
               f"start={self.start}, finish={self.finish})"
