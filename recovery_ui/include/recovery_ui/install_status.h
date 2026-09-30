/*
 * SPDX-FileCopyrightText: The uwuAOSP Project
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

namespace recovery_ui {
// Presentation only. Installation decisions continue to use InstallResult and IsTextVisible().
enum class InstallStage { NONE, WAITING, VERIFYING, INSTALLING, SUCCESS, ERROR, CANCELLED };
}
