import datetime
import logging
import string
from typing import List, Optional

from joblib import Parallel, delayed

from bot_utils.team import Team
from clients.resps import RespsClient
from templaters.abstract import AbstractTemplater


class TeamsCache:
    def __init__(self, timestamp: datetime.datetime):
        self.team_data = {}
        self.single_string_output = []
        self.timestamp = timestamp
        self.team_names = sorted(RespsClient().get_all_team_names())

    def _generate_teams(self, templater: AbstractTemplater) -> List[Team]:
        # With parallel team-data generation function works ~6x faster
        return Parallel(15, backend="threading")(
            delayed(Team)(team, self.timestamp, templater) for team in self.team_names
        )

    def generate_cache(self, templater: AbstractTemplater, full=True):
        for team in self._generate_teams(templater):
            self.team_data[team.name] = team
        if full:
            self._build_call_tree()
            self._generate_single_string_output_cache()

    def _generate_single_string_output_cache(self):
        for _, data in self.team_data.items():
            self.single_string_output.append(" ".join(data.single_string_output))

        # Splitting final output for two messages due to symbols limit
        first_part = self.single_string_output[: len(self.single_string_output) // 2]
        second_part = self.single_string_output[len(self.single_string_output) // 2 :]
        # If we have <= 20 teams, don't split them onto two messages
        # telegram message size limit allows sending single message
        if len(first_part) < 20:
            self.single_string_output = ["\n".join(self.single_string_output)]
        else:
            self.single_string_output.clear()
            self.single_string_output.append("\n".join(first_part))
            self.single_string_output.append("\n".join(second_part))

    def _build_call_tree(self):
        self.call_tree = {}
        for team in self.team_data.values():
            self.call_tree[team.name] = team.name
            for alias in team.alias_list:
                if alias.strip():
                    self.call_tree[alias] = team.name
        logging.debug(self.call_tree)

    def find_closest_team(self, arg):
        arg = arg.strip(string.punctuation)
        from_tree = self.call_tree.get(arg)
        if from_tree:
            return from_tree

    def get_team_data(self, team_name: str) -> Optional[Team]:
        return self.team_data.get(team_name)
