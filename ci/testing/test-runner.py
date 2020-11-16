import os
import platform
import subprocess
import sys

try:
    import win32api
except ImportError:
    pass

from colorama import Fore, Style

VERSION_STR = 'v0.2.0'

# noinspection SpellCheckingInspection
SEM_FAILCRITICALERRORS = 0x0001
# noinspection SpellCheckingInspection
SEM_NOGPFAULTERRORBOX = 0x0002

error_code_map = {
    0x00000000: 'WAIT_0',
    0x00000080: 'ABANDONED_WAIT_0',
    0x000000C0: 'USER_APC',
    0x00000102: 'TIMEOUT',
    0x00000103: 'PENDING',
    0x00010001: 'DBG_EXCEPTION_HANDLED',
    0x00010002: 'DBG_CONTINUE',
    0x40000005: 'SEGMENT_NOTIFICATION',
    0x40000015: 'FATAL_APP_EXIT',
    0x40010003: 'DBG_TERMINATE_THREAD',
    0x40010004: 'DBG_TERMINATE_PROCESS',
    0x40010005: 'DBG_CONTROL_C',
    0x40010006: 'DBG_PRINTEXCEPTION_C',
    0x40010007: 'DBG_RIPEXCEPTION',
    0x40010008: 'DBG_CONTROL_BREAK',
    0x40010009: 'DBG_COMMAND_EXCEPTION',
    0x80000001: 'GUARD_PAGE_VIOLATION',
    0x80000002: 'DATATYPE_MISALIGNMENT',
    0x80000003: 'BREAKPOINT',
    0x80000004: 'SINGLE_STEP',
    0x80000026: 'LONGJUMP',
    0x80000029: 'UNWIND_CONSOLIDATE',
    0x80010001: 'DBG_EXCEPTION_NOT_HANDLED',
    0xC0000005: 'ACCESS_VIOLATION',
    0xC0000006: 'IN_PAGE_ERROR',
    0xC0000008: 'INVALID_HANDLE',
    0xC000000D: 'INVALID_PARAMETER',
    0xC0000017: 'NO_MEMORY',
    0xC000001D: 'ILLEGAL_INSTRUCTION',
    0xC0000025: 'NONCONTINUABLE_EXCEPTION',
    0xC0000026: 'INVALID_DISPOSITION',
    0xC000008C: 'ARRAY_BOUNDS_EXCEEDED',
    0xC000008D: 'FLOAT_DENORMAL_OPERAND',
    0xC000008E: 'FLOAT_DIVIDE_BY_ZERO',
    0xC000008F: 'FLOAT_INEXACT_RESULT',
    0xC0000090: 'FLOAT_INVALID_OPERATION',
    0xC0000091: 'FLOAT_OVERFLOW',
    0xC0000092: 'FLOAT_STACK_CHECK',
    0xC0000093: 'FLOAT_UNDERFLOW',
    0xC0000094: 'INTEGER_DIVIDE_BY_ZERO',
    0xC0000095: 'INTEGER_OVERFLOW',
    0xC0000096: 'PRIVILEGED_INSTRUCTION',
    0xC00000FD: 'STACK_OVERFLOW',
    0xC0000135: 'DLL_NOT_FOUND',
    0xC0000138: 'ORDINAL_NOT_FOUND',
    0xC0000139: 'ENTRYPOINT_NOT_FOUND',
    0xC000013A: 'CONTROL_C_EXIT',
    0xC0000142: 'DLL_INIT_FAILED',
    0xC00002B4: 'FLOAT_MULTIPLE_FAULTS',
    0xC00002B5: 'FLOAT_MULTIPLE_TRAPS',
    0xC00002C9: 'REG_NAT_CONSUMPTION',
    0xC0000409: 'STACK_BUFFER_OVERRUN',
    0xC0000417: 'INVALID_CRUNTIME_PARAMETER',
    0xC0000420: 'ASSERTION_FAILURE',
    0xC015000F: 'SXS_EARLY_DEACTIVATION',
    0xC0150010: 'SXS_INVALID_DEACTIVATION',
}


class TestFile:
    def __init__(self, root: str, file_name: str):
        self.file_name = file_name
        self.full_path = f'{root}/{file_name}'
        self.output = ''
        self.exit_code = 0

    def run(self):
        extra = ''
        if use_llvm is True:
            extra += '--llvm-api'

        process = subprocess.Popen(["odin", "run", self.full_path, extra],
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                   universal_newlines=True)
        self.output, _ = process.communicate()
        self.exit_code = process.returncode


class TestCollection:
    def __init__(self, root: str, files: [str]):
        self.root_path = root
        files = [f for f in files if f.endswith(".odin") is True]
        self.test_files = []
        for f in files:
            self.test_files.append(TestFile(root, f))

    def run(self):
        for test in self.test_files:
            print(f"--{test.file_name} ...", end=' ')
            test.run()
            if test.exit_code != 0:
                print(f'{Fore.RED}FAIL ({test.exit_code}){Style.RESET_ALL}')
            else:
                print(f'{Fore.GREEN}SUCCESS{Style.RESET_ALL}')


def get_total_test_files(test_collections: [TestCollection]) -> int:
    total: int = 0
    for col in test_collections:
        total += len(col.test_files)
    return total


def print_failed_summary(failed_tests):
    print(f'{Fore.RED}Failed tests summary:{Style.RESET_ALL}')
    for ft in failed_tests:
        print(f"-- {ft.file_name}")
        print(f'Error Code: {Fore.RED}{ft.exit_code}{Style.RESET_ALL}', end='')
        if platform.system() == 'Windows':
            print(f' -> {error_code_map.get(ft.exit_code, "No translation found")}')
        else:
            print('\n')
        print(f'Output: ', end='')
        if len(ft.output) > 0:
            print(f'\n{Style.DIM}== START OF OUTPUT =={Style.RESET_ALL}')
            print(ft.output)
            print(f'{Style.DIM}== END OF OUTPUT =={Style.RESET_ALL}')
        else:
            print("No output...")
        print()

use_llvm = False

def main():
    if platform.system() == 'Windows':
        win32api.SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX)

    
    root_test_dir = sys.argv[1]
    if len(sys.argv) > 2:
        global use_llvm
        use_llvm = bool(sys.argv[2])

    print(f'Odin test runner {VERSION_STR}')
    print(f'Looking for tests in \'{root_test_dir}\'...')

    test_collections = []

    for root, dirs, files in os.walk(root_test_dir):
        test_collections.append(TestCollection(root, files))

    total_tests = get_total_test_files(test_collections)
    print(f"Found {total_tests} test files to run!")
    print()

    for col in test_collections:
        print(f'Running test collection in {col.root_path}')
        col.run()
        print()

    failed_tests = [f for col in test_collections for f in col.test_files if f.exit_code > 0]

    if len(failed_tests) > 0:
        print_failed_summary(failed_tests)
        print(f'{Fore.RED}{len(failed_tests)} out of {total_tests} tests Failed! Do better!{Style.RESET_ALL}')
        exit(1)
    else:
        print(f'{Fore.GREEN}All {total_tests} tests passed! Good job!{Style.RESET_ALL}')
        exit(0)


if __name__ == '__main__':
    main()
