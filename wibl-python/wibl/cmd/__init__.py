import os
import sys


def get_subcommand_prog() -> str:
    return f"{os.path.basename(sys.argv[0])} {sys.argv[1]}"
