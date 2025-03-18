import datetime
import os
from pathlib import Path
import pandas
from toloka.streaming.observer import BaseObserver

class DecisionTracker(BaseObserver):
    """Stores all decisions and votes for labels on disk
    """

    def __init__(self) -> None:
        self.name = 'decision_tracker'
        self.events = []
        self.answers = []
        self.decision_file_name = 'logs/decisions.tsv'
        self.answers_file_name = 'logs/answers.tsv'

        self._events_fields = ['date_time', 'name', 'address', 'pedestrian_assignment_id', 'handler', 'group', 'event', 'comment']
        self._answers_fields = ['pedestrian_assignment_id', 'handler', 'group', 'url', 'label', 'skill', 'performer', 'result']

    async def __call__(self):
        pass

    def add_event(self, event_dict):
        self.events.append({**event_dict, 'date_time': datetime.datetime.utcnow().isoformat()})

    def add_answers(self, handler, answers, skills):
        copied_answers = answers.copy(deep=True)
        copied_answers['handler'] = handler
        copied_answers = copied_answers.join(skills.rename('skill'), on='performer')
        self.answers.append(copied_answers)

    async def should_resume(self) -> bool:
        # store decisions
        if self.events:
            list_tuples = []
            for event in self.events:
                list_tuples.append((
                    event['date_time'],
                    event['name'],
                    event['address'],
                    event['pedestrian_assignment_id'],
                    event['handler'],
                    event['group'],
                    event['event'],
                    event['comment']
                ))
            new_df = pandas.DataFrame(list_tuples, columns=self._events_fields)
            if os.path.isfile(self.decision_file_name):
                events_df = pandas.read_csv(self.decision_file_name, sep='\t')
                events_df = events_df.append(new_df, ignore_index=True)
            else:
                events_df = new_df
            events_df.to_csv(self.decision_file_name, sep='\t', index=False)
            self.events = []

        # store answers
        if self.answers:
            if os.path.isfile(self.answers_file_name):
                all_answers_df = pandas.read_csv(self.answers_file_name, sep='\t')
            else:
                all_answers_df = pandas.DataFrame([], columns=self._answers_fields)

            for some_answers_df in self.answers:
                for column_name in self._answers_fields:
                    if column_name not in some_answers_df.columns:
                        some_answers_df[column_name] = None
                all_answers_df = all_answers_df.append(some_answers_df, ignore_index=True)

            all_answers_df.to_csv(self.answers_file_name, sep='\t', index=False)
            self.answers = []

        # never stops pipeline
        return True
