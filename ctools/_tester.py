# Copyright 2019 ko-han. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
import os


def _show_info(test_args):
    import ctools
    print("Python: ", sys.executable)
    print('Ctools:', ctools.__version__)
    print("Arguments:", " ".join(test_args))


class Tester(object):
    def __init__(self, module_name):
        self.module_name = module_name

    def __call__(self, extra_argv=None, coverage=False, include_sys_argv=True, tests=None, verbose=True):
        import pytest

        module = sys.modules[self.module_name]
        module_path = os.path.abspath(module.__path__[0])
        test_args = []

        if extra_argv:
            test_args.extend(extra_argv)

        if coverage:
            test_args.append("--cov=" + module_path)

        if tests is None:
            tests = [self.module_name]

        if include_sys_argv and len(sys.argv) > 1:
            test_args.extend(sys.argv[1:])

        test_args += ["--pyargs"] + list(tests)

        _show_info(test_args)
        if verbose and "-v" not in test_args:
            test_args.append("-v")

        try:
            code = pytest.main(test_args)
        except SystemExit as exc:
            code = exc.code

        return code
