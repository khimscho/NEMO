from abc import ABC
from enum import Flag, auto
from typing import Dict, Any, Optional

from wibl.core.logger_file import DataPacket


class AlgorithmPhase(Flag):
    ON_LOAD = auto()
    AFTER_TIME_INTERP = auto()
    AFTER_GEOJSON_CONVERSION = auto()


class WiblAlgorithm(ABC):
    def __init__(self, phases: AlgorithmPhase, params: str, config: Dict[str, Any]):
        self.phases = phases
        self.params = params
        self.config = config

    def run_on_load(self, packet: DataPacket) -> Optional[DataPacket]:
        pass

    def run_after_time_interp(self, data: Dict[str, Any]) -> Dict[str, Any]:
        pass

    def run_after_geojson_conversion(self, data: Dict[str, Any]) -> Dict[str,Any]:
        pass
