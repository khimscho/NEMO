from abc import ABC
from enum import Flag, auto
from typing import Dict, Any, Optional

from wibl.core.logger_file import DataPacket


class AlgorithmPhase(Flag):
    ON_LOAD = auto()
    AFTER_TIME_INTERP = auto()
    AFTER_GEOJSON_CONVERSION = auto()


class WiblAlgorithm(ABC):
    name: str
    phases: AlgorithmPhase

    @classmethod
    def run_on_load(cls, data: DataPacket,
                    params: str, config: Dict[str, Any]) -> Optional[DataPacket]:
        pass

    @classmethod
    def run_after_time_interp(cls, data: Dict[str, Any],
                              params: str, config: Dict[str, Any]) -> Dict[str, Any]:
        pass

    @classmethod
    def run_after_geojson_conversion(cls, data: Dict[str, Any],
                                     params: str, config: Dict[str, Any]) -> Dict[str,Any]:
        pass
