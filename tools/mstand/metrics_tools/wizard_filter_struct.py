from typing import List
from typing import Optional
from typing import Set


class FilterType:
    ANY = "any"
    ALL = "all"


class WizardFilter:
    def __init__(
            self,
            slices: Optional[Set[str]] = None,
            only_with_slices: bool = False,
            only_without_slices: bool = False,

            plugins: Optional[Set[str]] = None,
            plugins_has_data: bool = False,
            only_with_plugins: bool = False,
            only_without_plugins: bool = False,

            baobab: Optional[Set[str]] = None,
            baobab_subtype: Optional[Set[str]] = None,
            baobab_children_paths: Optional[Set[str]] = None,
            baobab_features: Optional[Set[str]] = None,
            baobab_alignments: Optional[Set[str]] = None,
            only_with_baobab: bool = False,
            only_without_baobab: bool = False,

            wizard_types: Optional[Set[str]] = None,
            only_with_wizard_types: bool = False,
            only_without_wizard_types: bool = False,

            component_type: Optional[str] = None,

            filter_type: str = FilterType.ALL
    ):
        if only_without_slices:
            assert not slices and not only_with_slices, "cannot use slices in only_without_slices mode"
        if only_without_plugins:
            assert not plugins and not only_with_plugins, "cannot use plugins in only_without_plugins mode"
        if only_without_baobab:
            assert not baobab and not only_with_baobab, "cannot use baobab_wizard_names in only_without_baobab mode"
        if only_without_wizard_types:
            assert not wizard_types and not only_with_wizard_types, "cannot use wizard_types in only_without_wizard_types mode"

        self.slices = slices or set()
        self.only_with_slices = only_with_slices
        self.only_without_slices = only_without_slices

        self.plugins = plugins or set()
        self.plugins_has_data = plugins_has_data
        self.only_with_plugins = only_with_plugins
        self.only_without_plugins = only_without_plugins

        self.baobab = baobab or set()
        self.baobab_subtype = baobab_subtype or set()
        self.baobab_children_paths = baobab_children_paths or set()
        self.baobab_features = baobab_features or set()
        self.baobab_alignments = baobab_alignments or set()
        self.only_with_baobab = only_with_baobab
        self.only_without_baobab = only_without_baobab

        self.wizard_types = wizard_types or set()
        self.only_with_wizard_types = only_with_wizard_types
        self.only_without_wizard_types = only_without_wizard_types

        self.component_type = component_type

        self.filter_type = filter_type

    @staticmethod
    def merge_filters(
            filters: List['WizardFilter'],
            remove: Optional[List["str"]] = None,
    ) -> 'WizardFilter':
        remove = remove or ["baobab_subtype"]

        assert len(filters) > 0
        slices = filters[0].slices.copy()
        plugins = filters[0].plugins.copy()
        baobab = filters[0].baobab.copy()
        baobab_subtype = filters[0].baobab_subtype.copy()
        baobab_children_paths = filters[0].baobab_children_paths.copy()
        baobab_features = filters[0].baobab_features.copy()
        baobab_alignments = filters[0].baobab_alignments.copy()
        wizard_types = filters[0].wizard_types.copy()

        component_type = filters[0].component_type
        only_with_slices = filters[0].only_with_slices
        only_without_slices = filters[0].only_without_slices
        plugins_has_data = filters[0].plugins_has_data
        only_with_plugins = filters[0].only_with_plugins
        only_without_plugins = filters[0].only_without_plugins
        only_with_baobab = filters[0].only_with_baobab
        only_without_baobab = filters[0].only_without_baobab
        only_with_wizard_types = filters[0].only_with_wizard_types
        only_without_wizard_types = filters[0].only_without_wizard_types
        filter_type = filters[0].filter_type
        for filter in filters[1:]:
            slices.update(filter.slices)
            plugins.update(filter.plugins)
            baobab.update(filter.baobab)
            baobab_subtype.update(filter.baobab_subtype)
            baobab_children_paths.update(filter.baobab_children_paths)
            baobab_features.update(filter.baobab_features)
            baobab_alignments.update(filter.baobab_alignments)
            wizard_types.update(filter.wizard_types)

            assert component_type == filter.component_type
            assert only_with_slices == filter.only_with_slices
            assert only_without_slices == filter.only_without_slices
            assert plugins_has_data == filter.plugins_has_data
            assert only_with_plugins == filter.only_with_plugins
            assert only_without_plugins == filter.only_without_plugins
            assert only_with_baobab == filter.only_with_baobab
            assert only_without_baobab == filter.only_without_baobab
            assert only_with_wizard_types == filter.only_with_wizard_types
            assert only_without_wizard_types == filter.only_without_wizard_types
            assert filter_type == filter.filter_type

        if "baobab_subtype" in remove:
            baobab_subtype = None

        return WizardFilter(
            slices=slices,
            only_with_slices=only_with_slices,
            only_without_slices=only_without_slices,
            plugins=plugins,
            plugins_has_data=plugins_has_data,
            only_with_plugins=only_with_plugins,
            only_without_plugins=only_without_plugins,
            baobab=baobab,
            baobab_subtype=baobab_subtype,
            baobab_children_paths=baobab_children_paths,
            baobab_features=baobab_features,
            baobab_alignments=baobab_alignments,
            only_with_baobab=only_with_baobab,
            only_without_baobab=only_without_baobab,
            wizard_types=wizard_types,
            only_with_wizard_types=only_with_wizard_types,
            only_without_wizard_types=only_without_wizard_types,
            component_type=component_type,
            filter_type=filter_type,
        )

    def get_kwargs(self) -> dict:
        kwargs = {}
        if self.slices:
            kwargs["slices"] = sorted(self.slices)
        if self.only_with_slices:
            kwargs["only_with_slices"] = self.only_with_slices
        if self.only_without_slices:
            kwargs["only_without_slices"] = self.only_without_slices
        if self.plugins:
            kwargs["plugins"] = sorted(self.plugins)
        if self.plugins_has_data:
            kwargs["plugins_has_data"] = self.plugins_has_data
        if self.only_with_plugins:
            kwargs["only_with_plugins"] = self.only_with_plugins
        if self.only_without_plugins:
            kwargs["only_without_plugins"] = self.only_without_plugins
        if self.baobab:
            kwargs["baobab"] = sorted(self.baobab)
        if self.baobab_subtype:
            kwargs["baobab_subtype"] = sorted(self.baobab_subtype)
        if self.baobab_children_paths:
            kwargs["baobab_children_paths"] = sorted(self.baobab_children_paths)
        if self.baobab_features:
            kwargs["baobab_features"] = sorted(self.baobab_features)
        if self.baobab_alignments:
            kwargs["baobab_alignments"] = sorted(self.baobab_alignments)
        if self.only_with_baobab:
            kwargs["only_with_baobab"] = self.only_with_baobab
        if self.only_without_baobab:
            kwargs["only_without_baobab"] = self.only_without_baobab
        if self.wizard_types:
            kwargs["wizard_types"] = sorted(self.wizard_types)
        if self.only_with_wizard_types:
            kwargs["only_with_wizard_types"] = self.only_with_wizard_types
        if self.only_without_wizard_types:
            kwargs["only_without_wizard_types"] = self.only_without_wizard_types
        if self.component_type:
            kwargs["component_type"] = self.component_type
        if self.filter_type == FilterType.ANY:
            kwargs["filter_type"] = FilterType.ANY
        return kwargs

    def get_requirements(self) -> Set[str]:
        requirements = set()
        if self.slices or self.only_with_slices or self.only_without_slices:
            requirements.add("COMPONENT.json.slices")
        if self.baobab or self.only_with_baobab or self.only_without_baobab:
            requirements.add("COMPONENT.text.baobabWizardName")
        if self.baobab_subtype:
            requirements.add("COMPONENT.text.baobabWizardSubtype")
        if self.baobab_children_paths:
            requirements.add("COMPONENT.texts.baobabChildrenPaths")
        if self.baobab_features:
            requirements.add("COMPONENT.texts.baobabFeatures")
        if self.baobab_alignments:
            requirements.add("COMPONENT.texts.baobabPath")
        if self.plugins or self.only_with_plugins or self.only_without_plugins:
            requirements.add("COMPONENT.json.SearchRuntimeInfo")
        return requirements
