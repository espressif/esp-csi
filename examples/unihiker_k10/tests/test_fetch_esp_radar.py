# SPDX-FileCopyrightText: 2026 vb3227
# SPDX-License-Identifier: Apache-2.0

"""Offline dependency-integrity tests; fetch the official archive first."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('fetch_radar', ROOT / 'scripts/fetch_esp_radar.py')
FETCH = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(FETCH)


class FetchTests(unittest.TestCase):
    def test_corrupt_archive_does_not_overwrite_dependency(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive = root / 'corrupt.zip'
            archive.write_bytes(b'not an official archive')
            existing = root / 'lib/esp-radar/include/esp_radar.h'
            existing.parent.mkdir(parents=True)
            existing.write_bytes(b'keep this existing file')
            with self.assertRaisesRegex(ValueError, 'SHA-256 mismatch'):
                FETCH.install(root, archive)
            self.assertEqual(existing.read_bytes(), b'keep this existing file')

    def test_official_archive_installs_only_allowlisted_files(self):
        archive = ROOT / '.pio/downloads/esp-radar-0.2.0.zip'
        if not archive.is_file():
            self.skipTest('Run scripts/fetch_esp_radar.py first')
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            FETCH.install(root, archive)
            destination = root / 'lib/esp-radar'
            installed = {p.relative_to(destination).as_posix() for p in destination.rglob('*') if p.is_file()}
            self.assertEqual(installed, set(FETCH.FILES))


if __name__ == '__main__':
    unittest.main()
