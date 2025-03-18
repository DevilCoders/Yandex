# -*- coding: utf-8 -*

import postprocessing.criteria_api  # noqa
import postprocessing.compute_criteria_single
import user_plugins.plugin_key
import mstand_utils.mstand_module_helpers


class AutoCriteria(object):
    required_criteria_params = ("module_name", "class_name")
    criteria_params_field = "default_criteria"

    @classmethod
    def _get_criteria_params(cls, control_result):
        metric_name = control_result.metric_name()
        extra_data = control_result.extra_data

        if not isinstance(extra_data, dict):
            raise TypeError(
                "metric {} expected to have dictionary at extra_data"
                .format(metric_name)
            )

        raw_criteria_params = extra_data.get(cls.criteria_params_field)
        if not raw_criteria_params:
            raise ValueError(
                "metric {} missing {} section at metric result"
                .format(metric_name, cls.criteria_params_field)
            )

        criteria_params = {
            "module_name": raw_criteria_params.get("module_name"),
            "class_name": raw_criteria_params.get("class_name"),
            "source": raw_criteria_params.get("source"),
            "kwargs": raw_criteria_params.get("kwargs"),
        }

        for field in cls.required_criteria_params:
            if not criteria_params[field]:
                raise ValueError(
                    "metric {} missing {} at {} section"
                    .format(metric_name, field, cls.criteria_params_field)
                )

        return criteria_params

    def value(self, params):
        """
        :type params: postprocessing.criteria_api.CriteriaParamsForAPI
        :rtype: tuple[float, dict]
        """

        criteria_params = self._get_criteria_params(params.control_result)
        # TODO: criteria objects cache by criteria_params via frozendict
        criteria_object = mstand_utils.mstand_module_helpers.create_user_object(**criteria_params)
        assert not isinstance(criteria_object, self.__class__)

        raw_result = criteria_object.value(params)
        criteria_key = user_plugins.plugin_key.PluginKey.generate(
            criteria_params['module_name'], criteria_params['class_name']
        )
        return postprocessing.compute_criteria_single.parse_raw_criteria_result(
            criteria_key, raw_result, None
        )
