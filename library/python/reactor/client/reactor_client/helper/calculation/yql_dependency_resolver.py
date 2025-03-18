from reactor_client.helper.reaction.yql_reaction import YqlReactionBuilder


class YqlDependencyResolverBuilder(YqlReactionBuilder):
    def __init__(self):
        super(YqlDependencyResolverBuilder, self).__init__()
        super(YqlDependencyResolverBuilder, self).set_reaction_type("yql_operations", "resolve_dependencies", "1")
