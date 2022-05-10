
from typing import NamedTuple, NoReturn


class State:
    pass

class Timestamp:
    pass

class Writer:
    pass


class DataGenerator:
    """
    Encapsulate the requirements for generating NMEA data logged in binary format

    This provides a reference mechanism for generate NMEA messages from the simulator, and writing
    them into the output data file in the appropriate format.  This includes NMEA2000 and NMEA0183 data
    messages for the same position, time, and depths, so that the output data file is consistent with either
    data source.
    """
    def __init(self, nmea0183: bool = True, nmea2000: bool = True):
        """
        Default constructor, given the NMEA2000 object that's doing the data capture
        :param nmea0183:
        :param nmea2000:
        :return:
        """
        self.nmea0183 = nmea0183
        self.nmea2000 = nmea2000
        # Flag for verbose debug output
        self._m_verbose: bool = False
        # Timestamp information for the current output state
        self._m_now: Timestamp = None
        # Emit serial strings for NMEA0183
        self._m_serial: bool = False
        # Emit binary data for NMEA2000
        self._m_binary: bool = False

    def emit_time(self, state: State, writer: Writer) -> NoReturn:
        """
        Generate a timestamping message for the current state of the simulator
        :param state:
        :param writer:
        :return:
        """
        pass

    def emit_position(self, state: State, writer: Writer) -> NoReturn:
        """
        Generate messages for the current configuration of the simulator
        :param state:
        :param writer:
        :return:
        """
        pass

    def emit_depth(self, state: State, writer: Writer) -> NoReturn:
        """
        Generate a depth message from the current state of the simulator
        :param state:
        :param writer:
        :return:
        """
        pass

    def set_verbose(self, verb: bool):
        """
        Set verbose logging state
        :param verb:
        :return:
        """
        self._m_verbose = verb

    def _generate_system_time(self, state: State, output: Writer) -> NoReturn:
        """
        Generate NMEA2000 timestamp information
        :param state:
        :param output:
        :return:
        """
        pass

    def _generate_gnss(self, state: State, output: Writer) -> NoReturn:
        """
        Generate NMEA2000 positioning information
        :param state:
        :param output:
        :return:
        """
        pass

    def _generate_depth(self, state: State, output: Writer) -> NoReturn:
        """
        Generate NMEA2000 depth information
        :param state:
        :param output:
        :return:
        """
        pass

    def _generate_zda(self, state: State, output: Writer) -> NoReturn:
        """
        Generate NMEA0183 timestamp (ZDA) information
        :param state:
        :param output:
        :return:
        """
        pass

    def _generate_gga(self, state: State, output: Writer) -> NoReturn:
        """
        Generate NMEA0183 positioning (GPGGA) information
        :param state:
        :param output:
        :return:
        """
        pass

    def _generate_dbt(self, state: State, output: Writer) -> NoReturn:
        """
        Generate NMEA0183 depth (SDDBT) information
        :param state:
        :param output:
        :return:
        """
        pass

    def _compute_checksum(self, msg: str) -> int:
        """
        Compute a NMEA0183 sentence checksum
        :param msg:
        :return:
        """
        pass

    class FormattedAngle(NamedTuple):
        degrees: int
        minutes: float
        hemisphere: int

    def _format_angle(self, angle: float) -> FormattedAngle:
        """
        Convert an angle in decimal degrees into integer degrees and decimal minutes, with hemispehere indicator
        :param angle:
        :return: FormattedAngle
        """
        pass

    class MonthDay(NamedTuple):
        month: int
        day: int

    def _to_day_month(self, year: int, year_day: int) -> MonthDay:
        """
        Convert the current time into broken out form
        :param year:
        :param year_day:
        :return: MonthDay
        """


class Engine:
    """
    Run the core simulator for NMEA data output
    """
    def __init__(self, data_generator: DataGenerator):
        """
        Default constructor for a simulation engine
        """
        # current state information
        self._m_state = None
        # converter for state to output representation
        self._m_generator = data_generator

    def step_engine(self, write) -> int:
        """
        Move the state of the engine on to the next simulation time
        :param write:
        :return:
        """
        pass

    def _step_position(self, now: int) -> bool:
        """
        Step the position state to a given simulation time (if required)
        :param now:
        :return:
        """
        return False

    def _step_depth(self, now: int) -> bool:
        """
        Step the depth state to a given simulation time (if required)
        :param now:
        :return:
        """
        return False

    def _step_time(self, now: int) -> bool:
        """
        Step the real-world time state to a given simulation time (if required)
        :param now:
        :return:
        """
        return False
