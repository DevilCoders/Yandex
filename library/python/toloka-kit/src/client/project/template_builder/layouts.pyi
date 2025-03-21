__all__ = [
    'BaseLayoutV1',
    'BarsLayoutV1',
    'ColumnsLayoutV1',
    'CompareLayoutItem',
    'CompareLayoutV1',
    'SideBySideLayoutV1',
    'SidebarLayoutV1',
]
import toloka.client.project.template_builder.base
import toloka.util._extendable_enum
import typing


class BaseLayoutV1Metaclass(toloka.client.project.template_builder.base.VersionedBaseComponentMetaclass):
    @staticmethod
    def __new__(
        mcs,
        name,
        bases,
        namespace,
        **kwargs
    ): ...


class BaseLayoutV1(toloka.client.project.template_builder.base.BaseComponent, metaclass=BaseLayoutV1Metaclass):
    """Options for positioning elements in the interface, such as in columns or side-by-side.

    If you have more than one element in the interface, these components will help you arrange them the way you want.

    Attributes:
        validation: Validation based on condition.
    """

    def __init__(
        self,
        *,
        validation: typing.Optional[toloka.client.project.template_builder.base.BaseComponent] = None,
        version: typing.Optional[str] = '1.0.0'
    ) -> None:
        """Method generated by attrs for class BaseLayoutV1.
        """
        ...

    _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
    validation: typing.Optional[toloka.client.project.template_builder.base.BaseComponent]
    version: typing.Optional[str]


class BarsLayoutV1(BaseLayoutV1):
    """A component that adds top and bottom bars to the content.

    You can use other components inside each part of this component, such as images, text, or options.

    The top bar is located at the top edge of the component, and the bottom one is at the bottom edge. The content is
    placed between the bars and takes up all available space.

    Attributes:
        content: The main content.
        bar_after: The bar displayed at the bottom edge of the component.
        bar_before: The bar displayed at the top edge of the component.
        validation: Validation based on condition.
    """

    def __init__(
        self,
        content: typing.Optional[toloka.client.project.template_builder.base.BaseComponent] = None,
        *,
        bar_after: typing.Optional[toloka.client.project.template_builder.base.BaseComponent] = None,
        bar_before: typing.Optional[toloka.client.project.template_builder.base.BaseComponent] = None,
        validation: typing.Optional[toloka.client.project.template_builder.base.BaseComponent] = None,
        version: typing.Optional[str] = '1.0.0'
    ) -> None:
        """Method generated by attrs for class BarsLayoutV1.
        """
        ...

    _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
    validation: typing.Optional[toloka.client.project.template_builder.base.BaseComponent]
    version: typing.Optional[str]
    content: typing.Optional[toloka.client.project.template_builder.base.BaseComponent]
    bar_after: typing.Optional[toloka.client.project.template_builder.base.BaseComponent]
    bar_before: typing.Optional[toloka.client.project.template_builder.base.BaseComponent]


class ColumnsLayoutV1(BaseLayoutV1):
    """A component for placing content in columns.

    Use it to customize the display of content: set the column width and adjust the vertical alignment of content.

    Attributes:
        items: Columns to divide the interface into.
        full_height: Switches the component to column mode at full height and with individual scrolling. Otherwise, the
            height is determined by the height of the column that is filled in the most.
        min_width: The minimum width of the component; if it is narrower, columns are output sequentially, one by one.
        ratio: An array of values that specify the relative width of columns. For example, if you have 3 columns, the
            value [1,2,1] divides the space into 4 parts and the column in the middle is twice as large as the other
            columns.
            If the number of columns exceeds the number of values in the ratio property, the values are repeated. For
            example, if you have 4 columns and the ratio is set to [1,2], the result is the same as for [1,2,1,2].
            If the number of columns is less than the number of values in the ratio property, extra values are simply
            ignored.
        vertical_align: Vertical alignment of column content.
        validation: Validation based on condition.
    """

    class VerticalAlign(toloka.util._extendable_enum.ExtendableStrEnum):
        """Vertical alignment of column content.

        Attributes:
            TOP: Aligned to the top of a column.
            MIDDLE: Aligned to the middle of the column that is filled in the most.
            BOTTOM: Aligned to the bottom of a column.
        """

        BOTTOM = 'bottom'
        MIDDLE = 'middle'
        TOP = 'top'

    def __init__(
        self,
        items: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, typing.List[toloka.client.project.template_builder.base.BaseComponent]]] = None,
        *,
        full_height: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, bool]] = None,
        min_width: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, float]] = None,
        ratio: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, typing.List[typing.Union[toloka.client.project.template_builder.base.BaseComponent, float]]]] = None,
        vertical_align: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, VerticalAlign]] = None,
        validation: typing.Optional[toloka.client.project.template_builder.base.BaseComponent] = None,
        version: typing.Optional[str] = '1.0.0'
    ) -> None:
        """Method generated by attrs for class ColumnsLayoutV1.
        """
        ...

    _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
    validation: typing.Optional[toloka.client.project.template_builder.base.BaseComponent]
    version: typing.Optional[str]
    items: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, typing.List[toloka.client.project.template_builder.base.BaseComponent]]]
    full_height: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, bool]]
    min_width: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, float]]
    ratio: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, typing.List[typing.Union[toloka.client.project.template_builder.base.BaseComponent, float]]]]
    vertical_align: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, VerticalAlign]]


class CompareLayoutItem(toloka.client.project.template_builder.base.BaseTemplate):
    """The compared element.

    Attributes:
        content: The content of the element that's being compared. Add images, audio recordings, videos, links,
            or other types of data.
        controls: Configure the input fields to make the user select an item.
    """

    def __init__(
        self,
        content: typing.Optional[toloka.client.project.template_builder.base.BaseComponent] = None,
        controls: typing.Optional[toloka.client.project.template_builder.base.BaseComponent] = None
    ) -> None:
        """Method generated by attrs for class CompareLayoutItem.
        """
        ...

    _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
    content: typing.Optional[toloka.client.project.template_builder.base.BaseComponent]
    controls: typing.Optional[toloka.client.project.template_builder.base.BaseComponent]


class CompareLayoutV1(BaseLayoutV1):
    """Use it to arrange interface elements for comparing them. For example, you can compare several photos.

    Selection buttons can be placed under each of the compared items. You can also add common elements, such as a
    field for comments.

    Differences from layout.side-by-side:

    * No buttons for hiding items. These are useful if you need to compare 5 photos at once and it's
    difficult to choose between two of them.
    * You can add individual selection buttons for every item being compared.

    Attributes:
        common_controls: The common fields of the component. Add information blocks that are common to all the
            elements being compared.
        items: An array with properties of the elements being compared. Set the appearance of the component blocks.
        min_width: Minimum width of the element in pixels. Default: 400 pixels.
        wide_common_controls: This property increases the common field size of the elements being compared.
            It's set to false by default: the common fields are displayed in the center, not stretched. If true,
            the fields are wider than with the default value.
        validation: Validation based on condition.
    """

    def __init__(
        self,
        common_controls: typing.Optional[toloka.client.project.template_builder.base.BaseComponent] = None,
        items: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, typing.List[typing.Union[toloka.client.project.template_builder.base.BaseComponent, CompareLayoutItem]]]] = None,
        *,
        min_width: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, float]] = None,
        wide_common_controls: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, bool]] = None,
        validation: typing.Optional[toloka.client.project.template_builder.base.BaseComponent] = None,
        version: typing.Optional[str] = '1.0.0'
    ) -> None:
        """Method generated by attrs for class CompareLayoutV1.
        """
        ...

    _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
    validation: typing.Optional[toloka.client.project.template_builder.base.BaseComponent]
    version: typing.Optional[str]
    common_controls: typing.Optional[toloka.client.project.template_builder.base.BaseComponent]
    items: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, typing.List[typing.Union[toloka.client.project.template_builder.base.BaseComponent, CompareLayoutItem]]]]
    min_width: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, float]]
    wide_common_controls: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, bool]]


class SideBySideLayoutV1(BaseLayoutV1):
    """The component displays several data blocks of the same width on a single horizontal panel.

    For example, you can use this to compare several photos.

    You can set the minimum width for data blocks.

    Attributes:
        controls: Components that let users perform the required actions.
            For example: field.checkbox-group or field.button-radio-group.
        items: An array of data blocks.
        min_item_width: The minimum width of a data block, at least 400 pixels.
        validation: Validation based on condition.
    """

    def __init__(
        self,
        controls: typing.Optional[toloka.client.project.template_builder.base.BaseComponent] = None,
        items: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, typing.List[toloka.client.project.template_builder.base.BaseComponent]]] = None,
        *,
        min_item_width: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, float]] = None,
        validation: typing.Optional[toloka.client.project.template_builder.base.BaseComponent] = None,
        version: typing.Optional[str] = '1.0.0'
    ) -> None:
        """Method generated by attrs for class SideBySideLayoutV1.
        """
        ...

    _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
    validation: typing.Optional[toloka.client.project.template_builder.base.BaseComponent]
    version: typing.Optional[str]
    controls: typing.Optional[toloka.client.project.template_builder.base.BaseComponent]
    items: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, typing.List[toloka.client.project.template_builder.base.BaseComponent]]]
    min_item_width: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, float]]


class SidebarLayoutV1(BaseLayoutV1):
    """An option for placing (layout) items, which lets you arrange on a page:

    * The main content block.
    * An adjacent panel with controls.

    The minWidth property sets the threshold for switching between widescreen and compact modes: when the width of the
    layout.sidebar component itself becomes less than the value set by the minWidth property, compact mode is enabled.

    In widescreen mode, the control panel is located to the right of the main block.

    In compact mode, controls stretch to the entire width and are located under each other.

    To add an extra panel with controls, use the extraControls property.

    Attributes:
        content: Content placed in the main area.
        controls: Content of the control panel.
        controls_width: The width of the control panel in widescreen mode. In compact mode, the panel takes up the
            entire available width. Default: 200 pixels.
        extra_controls: An additional panel with controls. Located below the main panel.
        min_width: The minimum width, in pixels, for widescreen mode. If the component width becomes less than the
            specified value, the interface switches to compact mode. Default: 400 pixels.
        validation: Validation based on condition.
    """

    def __init__(
        self,
        content: typing.Optional[toloka.client.project.template_builder.base.BaseComponent] = None,
        controls: typing.Optional[toloka.client.project.template_builder.base.BaseComponent] = None,
        *,
        controls_width: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, float]] = None,
        extra_controls: typing.Optional[toloka.client.project.template_builder.base.BaseComponent] = None,
        min_width: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, float]] = None,
        validation: typing.Optional[toloka.client.project.template_builder.base.BaseComponent] = None,
        version: typing.Optional[str] = '1.0.0'
    ) -> None:
        """Method generated by attrs for class SidebarLayoutV1.
        """
        ...

    _unexpected: typing.Optional[typing.Dict[str, typing.Any]]
    validation: typing.Optional[toloka.client.project.template_builder.base.BaseComponent]
    version: typing.Optional[str]
    content: typing.Optional[toloka.client.project.template_builder.base.BaseComponent]
    controls: typing.Optional[toloka.client.project.template_builder.base.BaseComponent]
    controls_width: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, float]]
    extra_controls: typing.Optional[toloka.client.project.template_builder.base.BaseComponent]
    min_width: typing.Optional[typing.Union[toloka.client.project.template_builder.base.BaseComponent, float]]
