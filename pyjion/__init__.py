import ctypes
import pathlib
import os
import platform


def _no_dotnet(path):
    print(f"Can't find a .NET 5 installation in {path}, "
          "provide the DOTNET_ROOT environment variable "
          "if its installed somewhere unusual")
    exit(1)


# try and locate .Net 5
def _which_dotnet():
    _dotnet_root = pathlib.Path('/usr/local/share/dotnet/')
    if 'DOTNET_ROOT' in os.environ:
        _dotnet_root = pathlib.Path(os.environ['DOTNET_ROOT'])
    if not _dotnet_root.exists():
        _no_dotnet(_dotnet_root)
    if platform.system() == "Darwin":
        lib_path = list(_dotnet_root.glob('shared/Microsoft.NETCore.App*/5.0.0*/libclrjit.dylib'))
        if len(lib_path) > 0:
            clrjitlib = str(lib_path[0])
            ctypes.cdll.LoadLibrary(clrjitlib)
        else:
            _no_dotnet(_dotnet_root)
    elif platform.system() == "Linux":
        lib_path = list(_dotnet_root.glob('shared/Microsoft.NETCore.App*/5.0.0*/libclrjit.so'))
        if len(lib_path) > 0:
            clrjitlib = str(lib_path[0])
            ctypes.cdll.LoadLibrary(clrjitlib)
        else:
            _no_dotnet(_dotnet_root)
    else:
        # TODO : Implement Windows
        return


_which_dotnet()

from ._pyjion import *  # NOQA
