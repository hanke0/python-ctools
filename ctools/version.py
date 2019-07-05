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

__all__ = ["__version__"]


def safe_version(v):
    try:
        from pkg_resources import safe_version
        return safe_version(v)
    except ImportError:
        return v


_version = '0.1.0.beta1'


__version__ = safe_version(_version)
