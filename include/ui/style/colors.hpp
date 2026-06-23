#pragma once

#include <citro2d.h>

namespace lunar3d {
namespace ui {
namespace style {
namespace colors {

constexpr u32 TopBackground = C2D_Color32(17, 21, 27, 255);
constexpr u32 BottomBackground = C2D_Color32(23, 28, 36, 255);
constexpr u32 Text = C2D_Color32(235, 239, 245, 255);
constexpr u32 HintText = C2D_Color32(154, 165, 180, 255);
constexpr u32 Panel = C2D_Color32(35, 42, 53, 255);
constexpr u32 PanelAccent = C2D_Color32(52, 68, 88, 255);
constexpr u32 PanelPressed = C2D_Color32(63, 86, 112, 255);
constexpr u32 PanelDisabled = C2D_Color32(28, 33, 41, 255);
constexpr u32 Accent = C2D_Color32(84, 190, 142, 255);
constexpr u32 AccentDisabled = C2D_Color32(67, 78, 91, 255);
constexpr u32 Transparent = C2D_Color32(0, 0, 0, 0);

} // namespace colors
} // namespace style
} // namespace ui
} // namespace lunar3d
