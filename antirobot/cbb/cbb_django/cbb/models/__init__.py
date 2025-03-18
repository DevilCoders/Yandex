# Import shortcuts
from antirobot.cbb.cbb_django.cbb.models.block import BlockIPV4, BlockIPV6, BlockTXT, BlockRE  # noqa
from antirobot.cbb.cbb_django.cbb.models.history import HistoryIPV4, HistoryIPV6, HistoryTXT, HistoryRE, LimboIPV4, LimboIPV6, LimboTXT, LimboRE  # noqa
from antirobot.cbb.cbb_django.cbb.models.group import Group  # noqa
from antirobot.cbb.cbb_django.cbb.models.user_role import UserRole, GroupResponsible  # noqa

BLOCK_VERSIONS = {4: BlockIPV4, 6: BlockIPV6, "txt": BlockTXT, "re": BlockRE}
HISTORY_VERSIONS = {4: HistoryIPV4, 6: HistoryIPV6, "txt": HistoryTXT, "re": HistoryRE}
LIMBO_VERSIONS = {4: LimboIPV4, 6: LimboIPV6, "txt": LimboTXT, "re": LimboRE}
