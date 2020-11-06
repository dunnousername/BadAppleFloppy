#!/bin/env python3
import zlib

with open('../../encoder/encoded.bin', 'rb') as f:
    with open('blob.bin', 'wb') as g:
        g.write(zlib.compress(f.read(), level=9))