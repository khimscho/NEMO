
from typing import NoReturn

class State:
    pass

class DataGenerator:
    pass

class Engine:
    def __init__(self):
        self._m_state = None
        self._m_generator = None

    def step_engine(self, write) -> NoReturn:
        pass

    def _step_position(self, now: int) -> bool:
        return False

    def _step_depth(self, now: int) -> bool:
        return False

    def _step_time(self, now: int) -> bool:
        return False
