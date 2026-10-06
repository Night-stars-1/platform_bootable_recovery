/*
 * SPDX-FileCopyrightText: The uwuAOSP Project
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once
#include <string>
#include <vector>

namespace recovery_ui {
template <typename ReadProperty>
inline void AppendDeviceInfo(std::vector<std::string>& lines, ReadProperty property) {
  const std::string product = property("ro.product.device");
  std::string name;
  for (const char* key : {"ro.product.marketname", "ro.product.vendor.marketname",
                          "ro.product.odm.marketname", "bluetooth.device.default_name",
                          "ro.product.model"}) {
    name = property(key);
    if (!name.empty()) break;
  }
  if (name.empty()) name = product;
  if (!name.empty()) lines.push_back("Device name - " + name);
  lines.push_back("Product name - " + product);
}
}  // namespace recovery_ui
