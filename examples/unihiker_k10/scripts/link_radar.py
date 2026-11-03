# SPDX-FileCopyrightText: 2026 vb3227
# SPDX-License-Identifier: Apache-2.0

Import('env')
from pathlib import Path

root = Path(env.subst('$PROJECT_DIR'))
if not (root / 'lib/esp-radar/lib/esp32s3/libesp-radar.a').is_file():
    raise RuntimeError('Run python scripts/fetch_esp_radar.py before building this example')
env.Append(LIBPATH=[str(root / 'lib/esp-radar/lib/esp32s3')])
env.Append(LIBS=['esp-radar'])
