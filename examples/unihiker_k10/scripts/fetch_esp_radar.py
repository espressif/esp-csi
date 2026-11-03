# SPDX-FileCopyrightText: 2026 vb3227
# SPDX-License-Identifier: Apache-2.0

"""Fetch the pinned vendor component without tracking binary libraries in Git."""
import argparse
import hashlib
import io
from pathlib import Path
import urllib.request
import zipfile

URL = (
    'https://components.espressif.com/api/downloads/'
    '?object_id=6fcc076e-6963-4ce6-80c6-ae483f7d1669&object_type=component'
)
SHA256 = '97720566ad18a8d51c1af32fb06fb69490bf1cb137a97b68480c17137401c015'
FILES = ('include/esp_radar.h', 'lib/esp32s3/libesp-radar.a', 'LICENSE', 'idf_component.yml')


def install(root, archive=None):
    cache = root / '.pio/downloads/esp-radar-0.2.0.zip'
    if archive is not None:
        data = Path(archive).read_bytes()
    elif cache.is_file():
        data = cache.read_bytes()
    else:
        with urllib.request.urlopen(URL, timeout=30) as response:
            data = response.read(10 * 1024 * 1024 + 1)
    if hashlib.sha256(data).hexdigest() != SHA256:
        raise ValueError('Component SHA-256 mismatch; nothing was installed')
    with zipfile.ZipFile(io.BytesIO(data)) as package:
        # Only these exact entries are read; no archive-controlled extraction paths.
        payloads = {name: package.read('esp-radar/' + name) for name in FILES}
    destination = root / 'lib/esp-radar'
    for name, payload in payloads.items():
        path = destination / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(payload)
    cache.parent.mkdir(parents=True, exist_ok=True)
    cache.write_bytes(data)
    print('Installed verified esp-radar 0.2.0 (ESP32-S3); license in lib/esp-radar/LICENSE')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, help='Use a previously downloaded official archive')
    args = parser.parse_args()
    install(Path(__file__).resolve().parents[1], args.archive)
