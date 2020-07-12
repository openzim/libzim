import libzim_ext as libzim
import pytest

import os
from pathlib import Path
from itertools import product
import hashlib

DATADIR = Path(__file__).resolve().parent.parent/'data'


zim_files = filter(lambda p: p.suffix in ('.zim', '.zimaa'), DATADIR.glob('*.zim*'))


@pytest.fixture(params=zim_files)
def existing_zim_file(request):
    zim_path = request.param
    return zim_path.with_suffix('.zim')

def test_open_existing_zim(existing_zim_file):
    print("opening {}".format(existing_zim_file))
    f = libzim.File(str(existing_zim_file).encode())
    assert f.verify()
