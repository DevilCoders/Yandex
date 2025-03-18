from library.python.monitoring.solo.example.example_project.registry.project.projects import solo_example_project
from library.python.monitoring.solo.example.example_project.registry.sensor.sensors import cos, sin, exsecant, excosecant
from library.python.monitoring.solo.objects.solomon.v2 import Graph, Element, Selector

cos_graph = Graph(
    id=f"{cos.name}_graph",
    project_id=solo_example_project.id,
    name=f"{cos.name.capitalize()} sensor Graph",
    min="-1",
    max="1",
    transform="NONE",
    hide_no_data=False,
    normalize=False,
    ignore_inf=False,
    max_points=0,
    downsampling="BY_POINTS",
    scale="NATURAL",
    over_lines_transform="NONE",
    elements={
        Element(
            title=f"{cos.name.capitalize()}",
            selectors={Selector(name="sensor", value="{}".format(cos.name))}
        )
    }
)

sin_graph = Graph(
    id=f"{sin.name}_graph",
    project_id=solo_example_project.id,
    name=f"{sin.name.capitalize()} sensor Graph",
    min="-1",
    max="1",
    transform="NONE",
    hide_no_data=False,
    normalize=False,
    ignore_inf=False,
    max_points=0,
    downsampling="BY_POINTS",
    scale="NATURAL",
    over_lines_transform="NONE",
    elements={
        Element(
            title=f"{sin.name.capitalize()}",
            selectors={Selector(name="sensor", value="{}".format(sin.name))}
        )
    }
)

exsecant_graph = Graph(
    id=f"{exsecant.name}_graph",
    project_id=solo_example_project.id,
    name=f"{exsecant.name.capitalize()} sensor Graph",
    transform="NONE",
    hide_no_data=False,
    normalize=False,
    ignore_inf=False,
    max_points=0,
    downsampling="BY_POINTS",
    scale="NATURAL",
    over_lines_transform="NONE",
    elements={
        Element(
            title=f"{exsecant.name.capitalize()}",
            type="EXPRESSION",
            expression=f"{exsecant}"
        )
    }
)

excosecant_graph = Graph(
    id=f"{excosecant.name}_graph",
    project_id=solo_example_project.id,
    name=f"{excosecant.name.capitalize()} sensor Graph",
    transform="NONE",
    hide_no_data=False,
    normalize=False,
    ignore_inf=False,
    max_points=0,
    downsampling="BY_POINTS",
    scale="NATURAL",
    over_lines_transform="NONE",
    elements={
        Element(
            title=f"{excosecant.name.capitalize()}",
            type="EXPRESSION",
            expression=f"{excosecant}"
        )
    }
)

exports = [
    cos_graph,
    sin_graph,
    exsecant_graph,
    excosecant_graph,
]
