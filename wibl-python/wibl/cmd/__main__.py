import sys
import argparse

from wibl.cmd.edit_wibl_file import editwibl


class WIBL:
    def __init__(self):
        parser = argparse.ArgumentParser(
            description='Python tools for WIBL low-cost data logger system',
            usage='''wibl <command> [<arguments>]

    Commands include:
        datasim     Generate test data using Python-native data simulator.
        editwibl   Edit WIBL logger files, e.g., add platform metadata.
                '''
        )
        parser.add_argument('command', help='Subcommand to run')
        # TODO: Add version argument.
        # Only consume the command argument here, let sub-commands consume the rest
        args = parser.parse_args(sys.argv[1:2])
        if not hasattr(self, args.command):
            print(f"Unrecognized command: {args.command}")
            parser.print_help()
            sys.exit(1)
        # Call the subcommand
        getattr(self, args.command)()

    def datasim(self):
        raise NotImplementedError()

    def editwibl(self):
        editwibl()


def main():
    WIBL()
