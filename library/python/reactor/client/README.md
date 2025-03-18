This is the python-client for [Reactor](https://wiki.yandex-team.ru/nirvana/reactor/) HTTP API.
А detailed description of the Reactor HTTP API can be found [here](https://wiki.yandex-team.ru/nirvana/reactor/api/http/).

The client
- converts API json-objects to python-objects and vise versa
- validate types as much as possible
- retries HTTP requests
- provides high-level builders for [reactions](https://wiki.yandex-team.ru/nirvana/reactor/reaction/) and [artifacts](https://wiki.yandex-team.ru/nirvana/reactor/artifact/). You can see some examples of using builders in section [Examples](#examples).

**If the client does not support something the Reactor HTTP API can, please, contact us at [Reactor support mailing](mailto:reactor@yandex-team.ru).**

## Table of content
1. [Supported methods](#supported-methods)
1. [Builders](#builders)
1. [Examples](#examples)
    1. [Create new artifact version with client method](#create-new-artifact-version-with-client-method)
    1. [Create new artifact version with builder](#create-new-artifact-version-with-builder)
    1. [Create Nirvana-reaction with builder](#create-nirvana-reaction-with-builder)
    1. [Create Yql-reaction with builder](#create-yql-reaction-with-builder)


### Supported methods

**Disclaimer:** Actual supported methods are determined [in this class](https://a.yandex-team.ru/arc/trunk/arcadia/library/python/reactor/client/reactor_client/reactor_api.py?rev=r7994349#L1308).

Here all client methods listed. Use them as in example below.
```python
client.artifact_instance.instantiate(...)
client.namespace.check_exists(...)
```

* greet - required only for the first visit to Reactor for new users
* artifact_type
    * [get](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.artifacttype.get)
    * [list](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.artifacttype.list)
* artifact
    * [get](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.artifact.get)
    * [create](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.artifact.create)
    * check_exists (not an API method, just syntactic sugar)
* artifact_instance
    * [instantiate](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.artifactinstance.instantiate)
    * [last](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.artifactinstance.last)
    * [range](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.artifactinstance.range)
    * [get_status_history](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.artifactinstance.getstatushistory)
* artifact_trigger
    * [insert](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.trigger.insert)
* reaction_type
    * [list](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.reactiontype.list)
* reaction
    * [create](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.reaction.create)
    * [get](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.reaction.get) (response contains only reaction)
    * [get_queue](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.reaction.get) (response contains only queue)
    * check_exists (not an API method, just syntactic sugar)
    * [update](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.reaction.update)
* reaction_instance
    * [get](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.reactioninstance.get)
    * [get_status_history](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.reactioninstance.getstatushistory)
    * [list](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.reactioninstance.list)
    * list_statuses (not an API method, just syntactic sugar)
    * [cancel](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.reactioninstance.cancel)
* dynamic_trigger
    * [add](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.trigger.add)
    * [remove](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.trigger.remove)
    * [list](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.trigger.list)
    * [update](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.trigger.update)
* queue
    * [create](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.queue.create)
    * [get](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.queue.get)
    * check_exists (not an API method, just syntactic sugar)
    * [update](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.queue.update)
* namespace
    * [get](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.namespace.get)
    * check_exists (not an API method, just syntactic sugar)
    * [create](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.namespace.create)
    * [delete](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.namespace.delete)
    * [list](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.namespace.list)
    * [list_names](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.namespace.listnames)
    * [resolve_path](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.namespace.resolvepath)
    * [resolve](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.namespace.resolve) (only for [Projects](https://wiki.yandex-team.ru/nirvana/reactor/project/))
    * [move](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.namespace.move)
* permission
    * [change](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.namespacepermission.change)
    * [list](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.namespacepermission.list)
* namespace_notification
    * [change](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.namespacenotification.change)
    * [list](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.namespacenotification.list)
    * [change_long_running](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.reactioneventsoptions.looongrunningoptionschange)
    * [get_long_running](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.reactioneventsoptions.looongrunningoptionsget)
* metric
    * [create](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.metric.create)
    * [list](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.metric.list)
    * [update](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.metric.update)
    * [delete](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.metric.delete)
* quota
    * [get](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.quota.get)
    * [update](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.quota.update)
    * [delete](https://wiki.yandex-team.ru/nirvana/reactor/api/http/#method.quota.delete)
* calculation
    * [create](https://wiki.yandex-team.ru/users/babichev-av/calculation-api/#method.calculation.create)
    * [get](https://wiki.yandex-team.ru/users/babichev-av/calculation-api/#method.calculation.get)
    * [list_metadata](https://wiki.yandex-team.ru/users/babichev-av/calculation-api/#method.calculation.listmetadata)
    * [update](https://wiki.yandex-team.ru/users/babichev-av/calculation-api/#method.calculation.update)
    * [delete](https://wiki.yandex-team.ru/users/babichev-av/calculation-api/#method.calculation.delete)
    * [translate](https://wiki.yandex-team.ru/users/babichev-av/calculation-api/#method.calculation.translate)


### Builders

Client has some builders for more convenient reaction descriptors and artifact versions definition.
* *reactor_client.helper.artifact_instance.ArtifactInstanceBuilder* - builder for artifact versions definition
* *reactor_client.helper.reaction.blank_reaction.BlankReactionBuilder* - builder for [blank reaction](https://wiki.yandex-team.ru/nirvana/reactor/reaction/type/#blank).
* *reactor_client.reaction_builders.NirvanaReactionBuilder* - builder for [Nirvana-reaction](https://wiki.yandex-team.ru/nirvana/reactor/reaction/type/#nirvana).
* *reactor_client.helper.reaction.sandbox_reaction.SandboxReactionBuilder* - builder for [Sandbox-reaction](https://wiki.yandex-team.ru/nirvana/reactor/reaction/type/#sandbox).
* *reactor_client.helper.reaction.yql_reaction.YqlReactionBuilder* - builder for [Yql-reaction](https://wiki.yandex-team.ru/nirvana/reactor/reaction/type/#yql).

Those who want to use client to manage Reactor [regular caluclations](https://wiki.yandex-team.ru/users/babichev-av/regular-calculation/) client has builders for dependency-resolver reactions.
* *reactor_client.helper.calculation.sandbox_dependency_resolver.SandboxDependencyResolverBuilder* - dependency resolver based on Sandbox-task.
* *reactor_client.helper.calculation.yql_dependency_resolver.YqlDependencyResolverBuilder* - dependency resolver based on YQL-query.

### Examples

#### Create new artifact version with client method

Put link to table *//tmp/path/to/table* on cluster *hahn* to artifact *[/examples/artifacts/tmp_table](https://test.reactor.yandex-team.ru/browse/resolve?path=/examples/artifacts/tmp_table)* using client method *artifact_instance.instantiate()*.
Prepare arguments for method manually.

**Note**: this scenario is less convenient than using *ArtifactInstanceBuilder*. [Example](#create-new-artifact-version-with-builder).

```python
from datetime import datetime

from reactor_client.reactor_api import ReactorAPIClientV1
import reactor_client.reactor_objects as r_objects

client = ReactorAPIClientV1(base_url="https://test.reactor.yandex-team.ru", token="your reactor token")

artifact_id = r_objects.ArtifactIdentifier(
    namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="/examples/artifacts/tmp_table")
)

ytpath_meta = r_objects.Metadata(
    type_="/yandex.reactor.artifact.YtPathArtifactValueProto",
    dict_obj={
        "cluster": "hahn",
        "path": "//tmp/path/to/table"
    }
)

client.artifact_instance.instantiate(
    artifact_identifier=artifact_id,
    metadata=ytpath_meta,
    attributes=r_objects.Attributes({"stage": "test"}),
    user_time=datetime(2021, 1, 1, 12),
    create_if_not_exist=True  # creates new version if there is no one with specified user-time, otherwise returns existing one
)
```

#### Create new artifact version with builder

Put link to table *//tmp/path/to/table* on cluster *hahn* to artifact [/examples/artifacts/tmp_table](https://test.reactor.yandex-team.ru/browse/resolve?path=/examples/artifacts/tmp_table) using client method *artifact_instance.instantiate()*.
Prepare arguments for method with help of *ArtifactInstanceBuilder*.

```python
from datetime import datetime

from reactor_client.helper.artifact_instance import ArtifactInstanceBuilder
from reactor_client.reactor_api import ReactorAPIClientV1

client = ReactorAPIClientV1(base_url="https://test.reactor.yandex-team.ru", token="your reactor token")

artifact_instance = ArtifactInstanceBuilder()\
    .set_artifact_path("/examples/artifacts/tmp_table")\
    .set_yt_path("hahn", "//tmp/path/to/table")\
    .set_attributes({"stage": "test"})\
    .set_user_time(datetime(2021, 1, 1, 12))

client.artifact_instance.instantiate(
    artifact_identifier=artifact_instance.artifact_identifier,
    metadata=artifact_instance.value,
    attributes=artifact_instance.attributes,
    user_time=artifact_instance.user_time,
    create_if_not_exist=True  # creates new version if there is no one with specified user-time, otherwise returns existing one
)
```

#### Create Nirvana-reaction with builder

Create Nirvana-reaction for graph [get JSON and then sleep](https://test.nirvana.yandex-team.ru/flow/5894990a-e316-4892-acc2-96c5a1eeb501/e9da9ceb-ee98-4950-a472-64104fdc7729/graph) using client method *reaction.create()*.
Prepare reaction configuration with help of *NirvanaReactionBuilder*.
Bind reaction to project [/reactor/Project](https://test.nirvana.yandex-team.ru/browse/resolve?path=/reactor/Project).
Trigger reaction each time artifact [/examples/nirvana/trigger_event](https://test.nirvana.yandex-team.ru/browse/resolve?path=/examples/nirvana/trigger_event) new version appears.
Pass JSON from last version of artifact [/examples/nirvana/json](https://test.nirvana.yandex-team.ru/browse/resolve?path=/examples/nirvana/json).
Pass sleep time from attribute "sleep" of triggered artifact version.
Set sleep operation TTL as 10 min.
Reaction, created by this example: [/examples/nirvana/get_JSON_and_sleep](https://test.nirvana.yandex-team.ru/browse/resolve?path=/examples/nirvana/get_JSON_and_sleep)

**Note**: Nirvana-reaction builder became outdated and [requires refactoring](https://st.yandex-team.ru/REACTOR-1872).

```python
from reactor_client.reaction_builders import NirvanaReactionBuilder
from reactor_client.reactor_api import ReactorAPIClientV1
import reactor_client.reactor_objects as r_objects

client = ReactorAPIClientV1(base_url="https://test.reactor.yandex-team.ru", token="your reactor token")

builder = NirvanaReactionBuilder()
builder.set_reaction_path("/examples/nirvana/get_JSON_and_sleep", description="Test Nirvana-reaction by python-client for Reactor")
builder.set_project(r_objects.ProjectIdentifier(namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="/reactor/Project")))
builder.set_cleanup_strategy(r_objects.CleanupStrategyDescriptor(
    cleanup_strategies=[r_objects.CleanupStrategy(ttl_cleanup_strategy=r_objects.TtlCleanupStrategy(1))]
))

trigger_artifact_path = "/examples/nirvana/trigger_event"
builder.set_dynamic_triggers(triggers=[
    r_objects.DynamicTrigger(
        trigger_name='artifact_trigger',
        expression=r_objects.Expression("global sleep_time = Datum.integer(a'" + trigger_artifact_path + "'.triggered.getAttribute(\"sleep\"));"),
        artifact_trigger=r_objects.DynamicArtifactTrigger(
            triggers=[r_objects.ArtifactReference(namespace_identifier=r_objects.NamespaceIdentifier(namespace_path=trigger_artifact_path))]
        )
    )
])

builder.set_source_graph(instance_id="e9da9ceb-ee98-4950-a472-64104fdc7729")
builder.set_target_graph(flow_id="f3c5ff2f-493e-4800-bf1f-a6a074c31628")
builder.set_owner("robot-reactor-tester")
builder.set_quota("reactor")
builder.set_result_ttl(1)
builder.set_retry_policy(retries=2, time_param=100, retry_policy_descriptor=r_objects.RetryPolicyDescriptor.UNIFORM, result_cloning_policy=r_objects.NirvanaResultCloningPolicy.TOP_LEVEL)

builder.set_version(2) # this is required to enable passing inputs through expression variables
builder.set_free_block_input_to_artifact("getJson", "source", r_objects.ArtifactReference(namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="/examples/nirvana/json")))
builder.set_block_param_to_expression_var("sleep", "SleepSeconds", r_objects.ExpressionVariable("sleep_time"))
builder.set_block_param_to_value_with_hints("sleep", "ttl", 10, r_objects.VariableTypes.INT)

old_version = r_objects.OperationIdentifier(namespace_identifier=r_objects.NamespaceIdentifier(namespace_path=path))
client.reaction.create(builder.operation_descriptor, create_if_not_exist=True, replacement_kind=r_objects.ReactionReplacementKind.REPLACE_AND_PRESERVE_OLD_VERSION, old_version_for_replacement=old_version, replacement_description="Add cron trigger")
```

#### Create Yql-reaction with builder

Create YQL-reaction for request [insertFromTutorialTable](https://yql.yandex-team.ru/Queries/60d35b41bf788684397edd85) using client method *reaction.create()*.
Prepare reaction configuration with help of *YqlReactionBuilder*.
Bind reaction to project [/reactor/Project](https://test.nirvana.yandex-team.ru/browse/resolve?path=/reactor/Project).
Trigger reaction each time artifact [/examples/yql/trigger_event](https://test.nirvana.yandex-team.ru/browse/resolve?path=/examples/yql/trigger_event) new version appears.
Set as default name the last version of [/examples/yql/default_name](https://test.nirvana.yandex-team.ru/browse/resolve?path=/examples/yql/default_name).
Pass default ip from attribute "ip" of triggered artifact version.
Set table name for inserting as "test_tutorial_table".
Reaction, created by this example: [/examples/yql/insertFromTutorialTable](https://test.nirvana.yandex-team.ru/browse/resolve?path=/examples/yql/insertFromTutorialTable)

```python
from reactor_client.helper.reaction.yql_reaction import YqlReactionBuilder
from reactor_client.reactor_api import ReactorAPIClientV1
import reactor_client.reactor_objects as r_objects

client = ReactorAPIClientV1(base_url="https://test.reactor.yandex-team.ru", token="your reactor token")

trigger_artifact_path = "/examples/yql/trigger_event"
builder = YqlReactionBuilder()\
    .set_reaction_path("/examples/yql/insertFromTutorialTable", description="Test YQL-reaction by python-client for Reactor")\
    .set_project("/reactor/Project")\
    .set_reaction_instance_ttl_days(1)\
    .set_query_id("60d360dad67355acca6830dd")\
    .add_trigger_by_artifacts(
        'artifact_trigger',
        [r_objects.ArtifactReference(namespace_identifier=r_objects.NamespaceIdentifier(namespace_path=trigger_artifact_path))],
        expr="global default_ip = a'" + trigger_artifact_path + "'.triggered.getAttribute(\"ip\");"
    )\
    .set_uniform_retries(retry_number=2, delay_millis=100)\
    .add_input("$table", const="test_tutorial_table")\
    .add_input("$default_name", artifact_reference=r_objects.ArtifactReference(namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="/examples/yql/default_name")))\
    .add_input("$default_ip", expression_var="default_ip")

client.reaction.create(builder.operation_descriptor, create_if_not_exist=True)
```


