import datetime
import os
from pathlib import Path
from toloka.streaming.observer import BaseObserver

class CursorsSaver(BaseObserver):
    def __init__(self) -> None:
        self.cursors = {}
        self.name = 'cursors_saver'

    async def __call__(self):
        pass

    async def should_resume(self) -> bool:
        for cursor_name, curs_tuple in self.cursors.items():
            attr_name = curs_tuple[1]
            cursor = curs_tuple[0]
            with open(f'saves/{cursor_name}.txt', 'w') as f:
                f.write((getattr(cursor._request, attr_name) + datetime.timedelta(milliseconds=1)).isoformat())
        return False

    def track_cursor(self, name, cursor, attr_name):
        Path('saves').mkdir(parents=True, exist_ok=True)
        self.cursors[name] = (cursor, attr_name)
        file_name = f'saves/{name}.txt'
        if os.path.exists(file_name):
            with open(file_name) as f:
                setattr(cursor._request, attr_name, datetime.datetime.fromisoformat(f.readline()))
