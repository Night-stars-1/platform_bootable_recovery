/*
 * SPDX-FileCopyrightText: The uwuAOSP Project
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace recovery_ui {
// The caller owns locked credential memory. UI reads/draws through this view;
// it never copies the sequence into strings, logs or a second credential buffer.
class PatternInput {
 public:
  virtual ~PatternInput() = default;
  virtual size_t Size() const = 0;
  virtual uint8_t Cell(size_t index) const = 0;
  virtual bool Append(uint8_t cell) = 0;
  virtual void Clear() = 0;
  bool Contains(uint8_t cell) const {
    for (size_t i = 0; i < Size(); ++i) if (Cell(i) == cell) return true;
    return false;
  }
  bool Select(uint8_t cell) {
    if (cell >= 9 || Size() >= 9 || Contains(cell)) return false;
    if (Size()) {
      const auto previous = Cell(Size() - 1);
      int x1 = previous % 3, y1 = previous / 3, x2 = cell % 3, y2 = cell / 3;
      // Match Android LockPatternView's unvisited midpoint rule.
      if ((std::abs(x1 - x2) == 2 || std::abs(y1 - y2) == 2) &&
          (x1 + x2) % 2 == 0 && (y1 + y2) % 2 == 0) {
        uint8_t middle = ((y1 + y2) / 2) * 3 + (x1 + x2) / 2;
        if (!Contains(middle) && !Append(middle)) return false;
      }
    }
    return Append(cell);
  }
};
}  // namespace recovery_ui
