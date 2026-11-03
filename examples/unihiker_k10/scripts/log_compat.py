# SPDX-FileCopyrightText: 2026 vb3227
# SPDX-License-Identifier: Apache-2.0

Import('env')

# The vendor SDK inserts esp-insights log hooks. Keep the standard IDF logger
# for this standalone sensing firmware; cloud diagnostics are not initialized.
hooks = {'-Wl,--wrap=esp_log_write', '-Wl,--wrap=esp_log_writev'}
env.Replace(LINKFLAGS=[flag for flag in env['LINKFLAGS'] if flag not in hooks])
