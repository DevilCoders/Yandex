from .abstract_reaction import AbstractReactionBuilder


class BlankReactionBuilder(AbstractReactionBuilder):
    def __init__(self):
        super(BlankReactionBuilder, self).__init__()
        super(BlankReactionBuilder, self).set_reaction_type("miscellaneous", "blank_operation", "1")

    @property
    def parameters_values(self):
        """
        :rtype: dict[str, r_objs.ParametersValueElement]
        """
        return super(BlankReactionBuilder, self).parameters_values
