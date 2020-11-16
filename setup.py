from skbuild import setup
from packaging.version import LegacyVersion
from skbuild.exceptions import SKBuildError
from skbuild.cmaker import get_cmake_version

# Add CMake as a build requirement if cmake is not installed or is too low a version
setup_requires = []
try:
    if LegacyVersion(get_cmake_version()) < LegacyVersion("3.2"):
        setup_requires.append('cmake')
except SKBuildError:
    setup_requires.append('cmake')


setup(
    name='pyjion',
    version='1.0',
    description='A JIT compiler wrapper for CPython',
    author='Microsoft',
    license='MIT',
    packages=['pyjion'],
    setup_requires=setup_requires,
    include_package_data=True,
    #cmake_args=['-DDUMP_TRACES=1']
)
