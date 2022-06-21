import sys
import argparse

from wibl import __version__ as version
from wibl.cmd.edit_wibl_file import editwibl
from wibl.cmd.datasim import datasim
from wibl.cmd.upload_wibl_file import uploadwibl


class WIBL:
    def __init__(self):
        parser = argparse.ArgumentParser(
            description='Python tools for WIBL low-cost data logger system',
            usage='''wibl <command> [<arguments>]

    Commands include:
        datasim     Generate test data using Python-native data simulator.
        editwibl    Edit WIBL logger files, e.g., add platform metadata.
        uploadwibl  Upload WIBL logger files to an S3 bucket.
                '''
        )
        parser.add_argument('--version', help='print version and exit',
                            action='version', version=f"%(prog)s {version}")
        parser.add_argument('command', help='Subcommand to run')

        # Only consume the command argument here, let sub-commands consume the rest
        args = parser.parse_args(sys.argv[1:2])

        if not hasattr(self, args.command):
            print(f"Unrecognized command: {args.command}")
            parser.print_help()
            sys.exit(1)
        # Call the subcommand
        getattr(self, args.command)()

    def datasim(self):
        datasim()

    def editwibl(self):
        editwibl()

    def uploadwibl(self):
        uploadwibl()


def main():
    WIBL()
