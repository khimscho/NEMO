from abc import ABC
from enum import Flag, auto
from typing import Dict, Any, Optional, Union

from wibl.core.logger_file import DataPacket


class AlgorithmPhase(Flag):
    ON_LOAD = auto()
    AFTER_TIME_INTERP = auto()
    AFTER_GEOJSON_CONVERSION = auto()


class WiblAlgorithm(ABC):
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

    @classmethod
    def run(cls, phase: AlgorithmPhase,
            data: Union[DataPacket, Dict[str, Any]],
            params: str, config: Dict[str, Any]) -> \
            Union[Optional[DataPacket], Dict[str, Any]]:
        if phase is AlgorithmPhase.ON_LOAD and phase in cls.phases:
            if not isinstance(data, DataPacket):
                raise ValueError((f"data object for phase {AlgorithmPhase.ON_LOAD.name} "
                                  "must be of type DataPacket"))
            return cls.run_on_load(data, params, config)
        elif phase is AlgorithmPhase.AFTER_TIME_INTERP and phase in cls.phases:
            if not isinstance(data, dict):
                raise ValueError((f"data object for phase {AlgorithmPhase.AFTER_TIME_INTERP.name} "
                                  "must be of type Dict[str, any"))
            return cls.run_after_time_interp(data, params, config)
        elif phase is AlgorithmPhase.AFTER_GEOJSON_CONVERSION and phase in cls.phases:
            if not isinstance(data, dict):
                raise ValueError((f"data object for phase {AlgorithmPhase.AFTER_GEOJSON_CONVERSION.name} "
                                  "must be of type Dict[str, any"))
            return cls.run_after_geojson_conversion(data, params, config)
        else:
            raise ValueError(f"Unable to run algorithm for phase {phase} when phases is {cls.phases}")
