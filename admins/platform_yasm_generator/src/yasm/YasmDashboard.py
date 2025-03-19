from dataclasses import dataclass, field
import json
from typing import List, Union


class DashboardEncoder(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, (YasmChart,
                          YasmGraph,
                          YasmDashboard,
                          YasmGraphSignal)):
            return o.__dict__

        return json.JSONEncoder.default(self, o)


@dataclass
class YasmChart:
    row: int
    column: int
    title: str


@dataclass
class YasmGraphSignal:
    title: str
    tag: str
    name: str
    host: str = field(default="QLOUD")


@dataclass
class YasmGraph(YasmChart):
    width: int = field(default=1)
    height: int = field(default=1)
    signals: List[YasmGraphSignal] = field(default_factory=list)
    consts: list = field(default_factory=lambda: [])
    type: str = field(default='graph')


@dataclass
class YasmAlert(YasmChart):
    name: str
    showThresholds: bool = field(default=True)
    type: str = field(default='alert')
    width: int = field(default=1)
    height: int = field(default=1)


@dataclass()
class YasmDashboard:
    title: str = field(default=None)
    editors: List[str] = field(default=None)
    key: str = field(default=None)
    charts: List[Union[YasmAlert, YasmGraph]] = field(default_factory=list)

    type: str = field(default='panel')

    def toJson(self):
        return json.dumps(self, cls=DashboardEncoder)


def column_generator():
    col = 1
    while True:
        yield col
        col += 1


def cpu_graphs(row, dcs, sv) -> List[YasmGraph]:
    col = column_generator()
    return [
        YasmGraph(
            title='cpu',
            row=row,
            column=next(col),
            signals=[
                YasmGraphSignal(
                    title="usage",
                    tag=f"itype=qloud;prj={sv}.{sv}.{sv};component={sv}",
                    name="portoinst-cpu_usage_cores_tmmv"),
                YasmGraphSignal(
                    title="limit",
                    tag=f"itype=qloud;prj={sv}.{sv}.{sv};component={sv}",
                    name="portoinst-cpu_usage_cores_tmmv"),
            ]

        ),
        *[
            YasmGraph(
                title=f'cpu {dc}',
                row=row,
                column=next(col),
                signals=[
                    YasmGraphSignal(
                        title="usage",
                        tag=f"itype=qloud;geo={dc}prj={sv}.{sv}.{sv};component={sv}",
                        name="portoinst-cpu_usage_cores_tmmv"),
                    YasmGraphSignal(
                        title="limit",
                        tag=f"itype=qloud;geo={dc}prj={sv}.{sv}.{sv};component={sv}",
                        name="portoinst-cpu_usage_cores_tmmv"),
                ]

            ) for dc in dcs]
    ]


if __name__ == '__main__':
    print(cpu_graphs(1, ['man', 'iva'], "project"))
# gr = YasmDashboard(
#     title='my_tilte',
#     editors=['stepanar, bro'],
#     key="This is a key",
#     charts=[
#         YasmAlert(
#             name='My alert',
#             width=1,
#             height=1,
#             row=1,
#             col=2,
#             title='my_title'
#         ),
#         YasmGraph(
#             width=1,
#             height=1,
#             row=1,
#             col=2,
#             title='my_title',
#             signals=[
#                 YasmGraphSignal(
#                     title='MySignal',
#                     name='GraphNmae',
#                     tag='mytag'
#                 )
#             ]
#
#         )
#     ]
#
# )
# from pprint import pprint as pp
#
# print(gr.toJson())
