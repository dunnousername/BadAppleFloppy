#!/bin/env python3
import sys
if sys.version_info[0] < 3:
    print('default python version is < 3. retrying by invoking python3')
    sys.exit(1)

import zlib

with open('../../encoder/encoded.bin', 'rb') as f:
    with open('blob.bin', 'wb') as g:
        g.write(zlib.compress(f.read(), 9))