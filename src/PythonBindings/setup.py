#!/usr/bin/env python

# Distributed under the MIT License.
# See LICENSE.txt for details.

from distutils.core import setup

setup(
    name='spectre',
    version='${SpECTRE_VERSION}',
    description="Python bindings for SpECTRE",
    author="SXS collaboration",
    url="https://spectre-code.org",
    license="MIT",
    packages=['spectre'],
    install_requires=['h5py', 'numpy', 'scipy'],
    classifiers=[
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
)
