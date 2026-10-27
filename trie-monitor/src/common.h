/* SPDX-License-Identifier: AGPL-3.0-only
 *
 * Copyright (C) 2025-2026 IKERLAN, S.Coop
 *
 * Author: Charles-Alexis Lefebvre
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* common.h

Shared constants used across the monitor modules.
*/

#ifndef MONITOR_COMMON_H
#define MONITOR_COMMON_H

#define MAX_FILES           2048
#define MAX_LINES           1024
/* The trace matrix preallocates MAX_FILES x MAX_LINES lines of MAX_LINE_LEN
 * bytes (about 300 MB with these values), which fits in the 4 GB of the
 * Jetson Nano. The longest line of the traces has 59 characters, the longest
 * trace 547 lines. */
#define MAX_LINE_LEN        128

#define S_TO_NS             1000000000

#endif /* MONITOR_COMMON_H */
