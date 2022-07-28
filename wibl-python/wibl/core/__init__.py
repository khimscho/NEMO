import os


class EnvVarUndefinedException(Exception):
    def __init__(self, var_name: str):
        super().__init__(f"{var_name} is not a defined environment variable.")


def getenv(var_name: str) -> str:
    if var_name not in os.environ:
        raise EnvVarUndefinedException(var_name)
    return os.environ[var_name]
