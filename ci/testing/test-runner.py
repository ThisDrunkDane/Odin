import os
import subprocess
import win32api
import sys
from colorama import Style, Fore, init as colorama_init

SEM_FAILCRITICALERRORS = 0x0001
SEM_NOGPFAULTERRORBOX = 0x0002


class Test_Run:
    exit_code = 0
    path = ''

    def __init__(self, exit, path):
        self.exit_code = exit
        self.path = path


def run_test_files(root, files):
    failed_tests = []

    for f in files:
        path = root + '/' + f
        print("--", f, "....", end='')
        exit_code = subprocess.call(
            ["odin", "run", path], stderr=subprocess.STDOUT)
        if exit_code > 0:
            failed_tests.append(Test_Run(exit_code, path))
            print(Fore.RED, "FAIL", Style.RESET_ALL)
        else:
            print(Fore.GREEN, "Success", Style.RESET_ALL)

    return failed_tests


def main():
    win32api.SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX)

    failed_tests = []
    colorama_init()

    for root, dirs, files in os.walk("../../tests"):
        if len(files) <= 0:
            continue
        print(Style.BRIGHT, "Running", len(files),
              "tests in", root, Style.RESET_ALL)
        failed_runs = run_test_files(root, files)
        if len(failed_runs) > 0:
            failed_tests += failed_runs
        print()

    if len(failed_tests) > 0:
        print(Fore.RED, len(failed_tests), "tests failed!", Style.RESET_ALL)
        for ft in failed_tests:
            print("  --", ft.path, end=' ')
            print(Style.BRIGHT, "Exit Code:", Fore.RED,
                  ft.exit_code, Style.RESET_ALL)
        sys.exit(0)


if __name__ == '__main__':
    main()
