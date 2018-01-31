import libzim_ext as libzim
import pytest

import os
from pathlib import Path
from itertools import product
import hashlib

DATADIR = Path(__file__).resolve().parent/'data'

@pytest.fixture(params=product(
    (b'ZIM\x04', b''),
    (0x00, 0x01, 0x11, 0x30, 0xFF),
    range(0, 100, 10)
))
def wrong_zim(request, tmpdir):
    prefix, byte, file_size = request.param
    basename = 'prefix' if prefix else 'noprefix'
    filename = tmpdir/'{}_{}_{:x}.zim'.format(basename, file_size, byte)
    with open(str(filename), 'wb') as f:
        f.write(prefix)
        f.write(bytes([byte])*file_size)
    return filename


zim_files = filter(lambda p: p.suffix in ('.zim', '.zimaa'), DATADIR.glob('*.zim*'))


@pytest.fixture(params=zim_files)
def existing_zim_file(request):
    zim_path = request.param
    return zim_path.with_suffix('.zim')

def gen_empty_zim_content():
    content = bytes()
    content += b'ZIM\x04' # Magic
    content += b'\x05\x00\x00\x00' # Version
    content += bytes([0])*16 # uuid
    content += bytes([0])*4 # article count
    content += bytes([0])*4 # cluster count
    content += bytes([80] + [0]*7) # url ptr pos
    content += bytes([80] + [0]*7) # title ptr pos
    content += bytes([80] + [0]*7) # cluster ptr pos
    content += bytes([80] + [0]*7) # mimelist ptr pos
    content += bytes([0])*4 # main page index
    content += bytes([0])*4 # layout page index
    content += bytes([80] + [0]*7) # checksum pos
    md5sum = hashlib.md5(content).digest()
    content += md5sum
    return content


@pytest.fixture
def empty_zim_file(tmpdir):
    filename = tmpdir/'empty.zim'
    with open(str(filename), 'wb') as f:
        f.write(gen_empty_zim_content())
    return filename


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


def test_open_wrong_zim(wrong_zim):
    print("opening {}".format(wrong_zim))
    with pytest.raises(RuntimeError):
        libzim.File(str(wrong_zim).encode())


def test_open_nasty_empty_zim(nasty_empty_zim_file):
    print("opening {}".format(nasty_empty_zim_file))
    with pytest.raises(RuntimeError):
        libzim.File(str(nasty_empty_zim_file).encode())


def test_open_existing_zim(existing_zim_file):
    print("opening {}".format(existing_zim_file))
    f = libzim.File(str(existing_zim_file).encode())
    assert f.verify()

def test_open_empty_zim(empty_zim_file):
    print("opening {}".format(empty_zim_file))
    f = libzim.File(str(empty_zim_file).encode())
    assert f.verify()

def test_verify_wrong_checksum(wrong_checksum_empty_zim_file):
    print("opening {}".format(wrong_checksum_empty_zim_file))
    f = libzim.File(str(wrong_checksum_empty_zim_file).encode())
    assert not f.verify()
