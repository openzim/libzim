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

def gen_empty_zim_content():                          # CANNOT DELETE THIS YET
    content = bytes()                                 # CANNOT DELETE THIS YET
    content += b'ZIM\x04' # Magic                     # CANNOT DELETE THIS YET
    content += b'\x05\x00\x00\x00' # Version          # CANNOT DELETE THIS YET
    content += bytes([0])*16 # uuid                   # CANNOT DELETE THIS YET
    content += bytes([0])*4 # article count           # CANNOT DELETE THIS YET
    content += bytes([0])*4 # cluster count           # CANNOT DELETE THIS YET
    content += bytes([80] + [0]*7) # url ptr pos      # CANNOT DELETE THIS YET
    content += bytes([80] + [0]*7) # title ptr pos    # CANNOT DELETE THIS YET
    content += bytes([80] + [0]*7) # cluster ptr pos  # CANNOT DELETE THIS YET
    content += bytes([80] + [0]*7) # mimelist ptr pos # CANNOT DELETE THIS YET
    content += bytes([0])*4 # main page index         # CANNOT DELETE THIS YET
    content += bytes([0])*4 # layout page index       # CANNOT DELETE THIS YET
    content += bytes([80] + [0]*7) # checksum pos     # CANNOT DELETE THIS YET
    md5sum = hashlib.md5(content).digest()            # CANNOT DELETE THIS YET
    content += md5sum                                 # CANNOT DELETE THIS YET
    return content                                    # CANNOT DELETE THIS YET


def _nasty_offset_filter(offset):
    # Minor version
    if 6 <= offset < 8:
        return False

    # uuid
    if 8 <= offset < 24:
        return False

    # page and layout index
    if 64 <= offset < 72:
        return False

    return True

@pytest.fixture(params=filter(_nasty_offset_filter, range(80)))
def nasty_empty_zim_file(request, tmpdir):
    offset = request.param
    content = gen_empty_zim_content()
    content = content[:offset] + b'\xFF' + content[offset+1:]
    filename = tmpdir/'nasty_empty_{}.zim'.format(offset)
    with open(str(filename), 'wb') as f:
        f.write(content)
    return filename

@pytest.fixture
def wrong_checksum_empty_zim_file(tmpdir):
    content = gen_empty_zim_content()
    content = content[:85] +b'\xFF' + content[86:]
    filename = tmpdir/'wrong_checksum_empty.zim'
    with open(str(filename), 'wb') as f:
        f.write(content)
    return filename


def test_open_nasty_empty_zim(nasty_empty_zim_file):
    print("opening {}".format(nasty_empty_zim_file))
    with pytest.raises(RuntimeError):
        libzim.File(str(nasty_empty_zim_file).encode())


def test_open_existing_zim(existing_zim_file):
    print("opening {}".format(existing_zim_file))
    f = libzim.File(str(existing_zim_file).encode())
    assert f.verify()

def test_verify_wrong_checksum(wrong_checksum_empty_zim_file):
    print("opening {}".format(wrong_checksum_empty_zim_file))
    f = libzim.File(str(wrong_checksum_empty_zim_file).encode())
    assert not f.verify()
