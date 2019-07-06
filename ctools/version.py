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
import re

__all__ = ["__version__"]

VERSION_PATTEN = r"""
^\s*v?
(?:
    (?P<version>\d+\.\d+)         # minimum 'N.N'
    (?P<extraversion>(?:\.\d+)*)  # any number of extra '.N' segments
    (?P<pre>                                          # pre-release
        [-_\.]?
        (?P<prerel>(a|b|c|rc|alpha|beta|pre|preview))
        [-_\.]?
        (?P<preversion>[0-9]+)?
    )?
    (?P<post>                                         # post release
        (?:-(?P<postversion1>[0-9]+))
        |
        (?:
            [-_\.]?
            (?P<postrel>post|rev|r)
            [-_\.]?
            (?P<postversion2>[0-9]+)?
        )
    )?
    (?P<dev>                                          # dev release
        [-_\.]?
        (?P<devrel>dev)
        [-_\.]?
        (?P<devversion>[0-9]+)?
    )?
)
(?:\+(?P<local>[a-z0-9]+(?:[-_\.][a-z0-9]+)*))?       # local version
\s*$
"""


def safe_version(v):
    _regex = re.compile(VERSION_PATTEN, re.VERBOSE | re.IGNORECASE)
    match = _regex.match(v)
    if not match:
        raise ValueError("Invalid Version")

    parts = [match.group("version")]

    extra_version = match.group("extraversion")
    if extra_version:
        parts.append(extra_version)

    post_version1 = match.group("postversion1")
    if post_version1:
        parts.append(".post")
        parts.append(post_version1)

    postrel = match.group("postrel")
    if postrel:
        parts.append(".post")
        postrel_version = match.group("postversion2")
        if postrel_version:
            parts.append(postrel_version)
        else:
            parts.append("0")

    devrel = match.group("devrel")
    if devrel:
        parts.append(".dev")
        devversion = match.group("devversion")
        if devversion:
            parts.append(devversion)
        else:
            parts.append("0")
    localversion = match.group("local")
    if localversion:
        parts.append("+")
        parts.append(localversion)
    return "".join(parts)


_version = "0.1.0"

__version__ = safe_version(_version)
