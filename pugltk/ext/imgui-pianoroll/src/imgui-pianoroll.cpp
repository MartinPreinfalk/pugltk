#include "imgui-pianoroll/imgui-pianoroll.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "imgui.h"
#include "imgui_internal.h"

// Interner Kontext für den aktuellen Frame
struct PianoRollContext {
  ImVec2 CanvasPos;
  ImVec2 CanvasSize;
  ImDrawList* DrawList = nullptr;
  bool IsCanvasHovered = false;
};

static PianoRollContext g_PRCtx;
static constexpr float PIANO_KEY_OVERLAY_WIDTH = 40.0f;

// --- Hilfsfunktionen für Koordinatenumrechnung ---
static float BeatToCanvasX(double beat, float pixelsPerBeat, float scrollX) {
  return (float)(beat * pixelsPerBeat) - scrollX;
}

static float PitchToCanvasY(int pitch, float noteHeight, float scrollY) { return (127 - pitch) * noteHeight - scrollY; }

static double CanvasXToBeat(float canvasX, float pixelsPerBeat, float scrollX) {
  return (double)(canvasX + scrollX) / pixelsPerBeat;
}

static int CanvasYToPitch(float canvasY, float noteHeight, float scrollY) {
  return 127 - (int)std::floor((canvasY + scrollY) / noteHeight);
}

static bool IsBlackKey(int pitch) {
  int n = pitch % 12;
  return (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
}

static void ZoomHelper(float& state_var, float& scroll_value, float const total_size_in_pixel, float const zoomFactor,
                       float const min, float const max, float const normalized_zoom_center = 0.5f) {
  auto const old_state_var = state_var;
  auto const new_state_var = ImClamp(state_var * zoomFactor, min, max);
  if (old_state_var == new_state_var) {
    return;
  }
  auto const delta_0 = total_size_in_pixel / old_state_var;
  auto const delta_1 = total_size_in_pixel / new_state_var;
  auto const delta_scroll = normalized_zoom_center * (delta_0 - delta_1);
  scroll_value = (scroll_value / old_state_var + delta_scroll) * new_state_var;
  state_var = new_state_var;
}

// Interne Eingabeverarbeitung (Zoom, Scroll, Panning)
static void HandleCanvasInput(PianoRollState& state, ImVec2 canvas_size) {
  ImGuiIO& io = ImGui::GetIO();
  ImVec2 const mousePos = io.MousePos;
  ImVec2 const localMouse = ImVec2(mousePos.x - g_PRCtx.CanvasPos.x, mousePos.y - g_PRCtx.CanvasPos.y);

  if (g_PRCtx.IsCanvasHovered) {
    if (io.KeyCtrl) {
      if (io.MouseWheel != 0.0f) {
        float zoomFactor = (io.MouseWheel > 0) ? 1.1f : 0.909f;
        if (localMouse.x < PIANO_KEY_OVERLAY_WIDTH) {
          float const maxNoteHeight = std::max(8.0f, canvas_size.y / 12.0f);
          ZoomHelper(state.noteHeight, state.scrollVal.y, canvas_size.y, zoomFactor, 5.0f, maxNoteHeight,
                     localMouse.y / canvas_size.y);
        } else {
          ZoomHelper(state.pixelsPerBeat, state.scrollVal.x, canvas_size.x, zoomFactor, 20.0f, 500.0f,
                     localMouse.x / canvas_size.x);
        }
      }
    } else {
      if (io.MouseWheel != 0.0f) {
        if (io.KeyShift) {
          state.scrollVal.x -= io.MouseWheel * state.pixelsPerBeat;
        }
      }
      if (io.MouseWheelH != 0.0f) {
        state.scrollVal.x += io.MouseWheelH * state.pixelsPerBeat;
      }
    }

    if (state.editMode == PianoRollEditMode::None && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
      state.scrollVal.x -= io.MouseDelta.x;
      state.scrollVal.y -= io.MouseDelta.y;
    }
  }

  state.scrollVal.x = ImMax(-1.25f * PIANO_KEY_OVERLAY_WIDTH, state.scrollVal.x);
  float maxScrollY = (128.0f * state.noteHeight) - canvas_size.y;
  state.scrollVal.y = ImClamp(state.scrollVal.y, 0.0f, ImMax(0.0f, maxScrollY));
}

static void DrawGrid(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size, PianoRollState const& state) {
  draw_list->PushClipRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), true);

  for (int pitch = 0; pitch <= 127; ++pitch) {
    float const y = PitchToCanvasY(pitch, state.noteHeight, state.scrollVal.y);
    if (y + state.noteHeight < 0 || y > canvas_size.y) continue;

    ImVec2 const line_start = ImVec2(canvas_pos.x, canvas_pos.y + y);
    ImVec2 const line_end = ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + y);

    ImU32 const bg_color = IsBlackKey(pitch) ? IM_COL32(30, 30, 30, 255) : IM_COL32(42, 42, 42, 255);
    draw_list->AddRectFilled(line_start, ImVec2(line_end.x, line_start.y + state.noteHeight), bg_color);
    draw_list->AddLine(ImVec2(line_start.x, line_start.y + state.noteHeight),
                       ImVec2(line_end.x, line_end.y + state.noteHeight), IM_COL32(55, 55, 55, 255));
  }

  double const startBeat = CanvasXToBeat(0, state.pixelsPerBeat, state.scrollVal.x);
  double const endBeat = CanvasXToBeat(canvas_size.x, state.pixelsPerBeat, state.scrollVal.x);

  int const startBeatIdx = (int)std::floor(startBeat);
  int const endBeatIdx = (int)std::ceil(endBeat);

  for (int i = startBeatIdx; i <= endBeatIdx; ++i) {
    for (int j = 0; j < state.sub_beat_grid; ++j) {
      auto const sub_beat_offset = static_cast<float>(j) / state.sub_beat_grid;
      float const x = BeatToCanvasX(i + sub_beat_offset, state.pixelsPerBeat, state.scrollVal.x);
      ImVec2 const line_start = ImVec2(canvas_pos.x + x, canvas_pos.y);
      ImVec2 const line_end = ImVec2(canvas_pos.x + x, canvas_pos.y + canvas_size.y);

      bool const isBeat = (j == 0);
      bool const isBar = (i % state.beats_per_bar == 0);
      ImU32 line_color =
          isBeat ? (isBar ? IM_COL32(120, 120, 120, 255) : IM_COL32(90, 90, 90, 150)) : IM_COL32(60, 60, 60, 150);
      draw_list->AddLine(line_start, line_end, line_color, isBeat ? (isBar ? 2.0f : 1.5f) : 1.0f);
    }
  }

  draw_list->PopClipRect();
}

static void DrawPianoKeysOverlay(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size, float noteHeight,
                                 ImVec2 scrollVal) {
  draw_list->PushClipRect(canvas_pos, ImVec2(canvas_pos.x + PIANO_KEY_OVERLAY_WIDTH, canvas_pos.y + canvas_size.y),
                          true);

  for (int pitch = 0; pitch <= 127; ++pitch) {
    float y = PitchToCanvasY(pitch, noteHeight, scrollVal.y);
    if (y + noteHeight < 0 || y > canvas_size.y) continue;

    ImVec2 p_min = ImVec2(canvas_pos.x, canvas_pos.y + y);
    ImVec2 p_max = ImVec2(canvas_pos.x + PIANO_KEY_OVERLAY_WIDTH, canvas_pos.y + y + noteHeight);

    bool black = IsBlackKey(pitch);
    ImU32 keyCol = black ? IM_COL32(20, 20, 20, 255) : IM_COL32(220, 220, 220, 255);

    draw_list->AddRectFilled(p_min, p_max, keyCol);
    draw_list->AddRect(p_min, p_max, IM_COL32(80, 80, 80, 255));

    if (pitch % 12 == 0) {
      char buf[8];
      snprintf(buf, sizeof(buf), "C%d", (pitch / 12) - 1);
      draw_list->AddText(ImVec2(p_min.x + 4, p_min.y + 2), IM_COL32(100, 100, 100, 255), buf);
    }
  }

  draw_list->PopClipRect();
}

namespace ImGui {

void FitPianoRollVertical(PianoRollState& state, int minPitch, int maxPitch, const ImVec2& canvasSize,
                         float contentPadding) {
  (void)contentPadding;
  if (canvasSize.x <= 0.0f || canvasSize.y <= 0.0f) {
    return;
  }

  constexpr float kDefaultPitchMin = 48.0f;
  constexpr float kDefaultPitchMax = 84.0f;
  constexpr float kVerticalPaddingKeys = 2.0f;
  constexpr float kMinPitchSpan = 12.0f;

  if (minPitch > maxPitch) {
    minPitch = static_cast<int>(kDefaultPitchMin);
    maxPitch = static_cast<int>(kDefaultPitchMax);
  }

  minPitch = std::max(0, minPitch - static_cast<int>(kVerticalPaddingKeys));
  maxPitch = std::min(127, maxPitch + static_cast<int>(kVerticalPaddingKeys));

  auto const pitchSpan = std::max<int>(12, maxPitch - minPitch + 1);
  state.noteHeight = std::clamp(canvasSize.y / static_cast<float>(pitchSpan), 8.0f, 36.0f);
  state.noteHeight = std::min(state.noteHeight, canvasSize.y / kMinPitchSpan);
  state.scrollVal.y = std::max(0.0f, (127.0f - static_cast<float>(maxPitch)) * state.noteHeight);
}

void FitPianoRollHorizontal(PianoRollState& state, double totalBeats, const ImVec2& canvasSize,
                           float contentPadding) {
  if (canvasSize.x <= 0.0f || canvasSize.y <= 0.0f) {
    return;
  }

  constexpr float kHorizontalZoomOutFactor = 1.2f;
  auto const visibleWidth = std::max(1.0f, canvasSize.x - contentPadding);
  auto const durationBeats = std::max(1.0, totalBeats * kHorizontalZoomOutFactor);
  state.pixelsPerBeat = std::clamp(visibleWidth / static_cast<float>(durationBeats), 20.0f, 220.0f);
  state.scrollVal.x = 0.0f;
}

void FitPianoRollFullLength(PianoRollState& state, double fullLengthBeats, const ImVec2& canvasSize,
                           float contentPadding) {
  if (canvasSize.x <= 0.0f || canvasSize.y <= 0.0f) {
    return;
  }

  auto const visibleWidth = std::max(1.0f, canvasSize.x - contentPadding);
  auto const durationBeats = std::max(1.0, fullLengthBeats);
  state.pixelsPerBeat = std::clamp(visibleWidth / static_cast<float>(durationBeats), 20.0f, 220.0f);
  state.scrollVal.x = 0.0f;
}

void FitPianoRollToContent(PianoRollState& state, int minPitch, int maxPitch, double totalBeats,
                           const ImVec2& canvasSize, float contentPadding) {
  if (canvasSize.x <= 0.0f || canvasSize.y <= 0.0f) {
    return;
  }

  constexpr float kDefaultPitchMin = 48.0f;
  constexpr float kDefaultPitchMax = 84.0f;
  constexpr float kDefaultBeatSpan = 8.0f;

  if (minPitch > maxPitch) {
    minPitch = static_cast<int>(kDefaultPitchMin);
    maxPitch = static_cast<int>(kDefaultPitchMax);
    totalBeats = kDefaultBeatSpan;
  }

  FitPianoRollVertical(state, minPitch, maxPitch, canvasSize, contentPadding);
  FitPianoRollHorizontal(state, totalBeats, canvasSize, contentPadding);
}

bool BeginPianoRoll(const char* str_id, PianoRollState& state, const ImVec2& size, PianoRollControl flags) {
  PushID(str_id);

  BeginChild("PianoRollCanvas", size, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);

  auto const available_size = GetContentRegionAvail().x;
  auto const widget_size = std::min(100.0f, available_size / 4.0f);

  bool const any_control_visible =
      (flags & PianoRollControl_ShowPercussiveSwitch) || (flags & PianoRollControl_ShowSnapToGridSwitch) ||
      (flags & PianoRollControl_ShowBeatsPerBarSetting) || (flags & PianoRollControl_ShowSubBeatGridSetting) ||
      (flags & PianoRollControl_ShowFitControls);

  if (flags & PianoRollControl_ShowPercussiveSwitch) {
    ImGui::Checkbox("percussive notes", &state.percussive_notes);
    ImGui::SameLine();
  }
  if (flags & PianoRollControl_ShowSnapToGridSwitch) {
    ImGui::Checkbox("snap to grid", &state.snap_to_grid);
    ImGui::SameLine();
  }
  if (flags & PianoRollControl_ShowBeatsPerBarSetting) {
    ImGui::SetNextItemWidth(widget_size);
    if (ImGui::InputInt("beats per bar", &state.beats_per_bar)) {
      state.beats_per_bar = ImClamp(state.beats_per_bar, 1, 32);
    }
    ImGui::SameLine();
  }
  if (flags & PianoRollControl_ShowSubBeatGridSetting) {
    ImGui::SetNextItemWidth(widget_size);
    if (ImGui::InputInt("sub beat grid", &state.sub_beat_grid)) {
      state.sub_beat_grid = ImClamp(state.sub_beat_grid, 1, 32);
    }
    ImGui::SameLine();
  }
  if (flags & PianoRollControl_ShowFitControls) {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f));
    if (ImGui::SmallButton("V Fit")) {
      FitPianoRollVertical(state, state.fitMinPitch, state.fitMaxPitch, GetContentRegionAvail());
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("H Fit")) {
      FitPianoRollHorizontal(state, state.fitTotalBeats, GetContentRegionAvail());
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Full")) {
      FitPianoRollFullLength(state, state.fitPatternLengthBeats, GetContentRegionAvail());
    }
    ImGui::PopStyleVar(2);
    ImGui::SameLine();
  }
  if (any_control_visible) {
    ImGui::NewLine();
  }

  g_PRCtx.CanvasPos = GetCursorScreenPos();
  g_PRCtx.CanvasSize = GetContentRegionAvail();
  g_PRCtx.DrawList = GetWindowDrawList();
  g_PRCtx.IsCanvasHovered = IsWindowHovered();

  HandleCanvasInput(state, g_PRCtx.CanvasSize);
  DrawGrid(g_PRCtx.DrawList, g_PRCtx.CanvasPos, g_PRCtx.CanvasSize, state);

  g_PRCtx.DrawList->PushClipRect(
      g_PRCtx.CanvasPos, ImVec2(g_PRCtx.CanvasPos.x + g_PRCtx.CanvasSize.x, g_PRCtx.CanvasPos.y + g_PRCtx.CanvasSize.y),
      true);

  return true;
}

bool PianoRollCreateNote(PianoRollState& state, double* outBeat, int* outPitch, int* outVelocity, double* outDuration) {
  if (!g_PRCtx.IsCanvasHovered || state.activeNoteId != -1) return false;

  if (IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    ImVec2 mousePos = GetIO().MousePos;
    ImVec2 localMouse = ImVec2(mousePos.x - g_PRCtx.CanvasPos.x, mousePos.y - g_PRCtx.CanvasPos.y);

    if (localMouse.x < PIANO_KEY_OVERLAY_WIDTH) return false;

    double clickBeat = CanvasXToBeat(localMouse.x, state.pixelsPerBeat, state.scrollVal.x);
    int clickPitch = CanvasYToPitch(localMouse.y, state.noteHeight, state.scrollVal.y);

    clickBeat = ImMax(0.0, std::round(clickBeat / 0.25) * 0.25);
    clickPitch = ImClamp(clickPitch, 0, 127);

    if (outBeat) *outBeat = clickBeat;
    if (outPitch) *outPitch = clickPitch;
    if (outVelocity) *outVelocity = 100;
    if (outDuration) {
      auto const sub_beat = 1.0f / state.sub_beat_grid;
      *outDuration = sub_beat;
    }

    state.activeNoteId = -999;
    state.editMode = PianoRollEditMode::Resizing;
    return true;
  }

  return false;
}

PianoRollInteraction PianoRollNote(PianoRollState& state, uint32_t id, double* beat, int* pitch, int* velocity,
                                   double* duration, std::function<ImU32(bool, bool, bool, int)> f_color) {
  PianoRollInteraction resultFlags = PianoRollInteraction_None;
  bool const& isPercussive = state.percussive_notes;
  ImGuiIO& io = ImGui::GetIO();

  float centerX = BeatToCanvasX(*beat, state.pixelsPerBeat, state.scrollVal.x);
  float centerY = PitchToCanvasY(*pitch, state.noteHeight, state.scrollVal.y) + (state.noteHeight * 0.5f);
  float diamondRadius = state.noteHeight * 0.40f;

  float x1, x2, y1, y2;
  if (isPercussive) {
    x1 = centerX - diamondRadius;
    x2 = centerX + diamondRadius;
    y1 = centerY - diamondRadius;
    y2 = centerY + diamondRadius;
  } else {
    x1 = BeatToCanvasX(*beat, state.pixelsPerBeat, state.scrollVal.x);
    x2 = BeatToCanvasX(*beat + *duration, state.pixelsPerBeat, state.scrollVal.x);
    y1 = PitchToCanvasY(*pitch, state.noteHeight, state.scrollVal.y);
    y2 = y1 + state.noteHeight;
  }

  if (x2 < 0 || x1 > g_PRCtx.CanvasSize.x || y2 < 0 || y1 > g_PRCtx.CanvasSize.y) {
    return resultFlags;
  }

  ImVec2 mousePos = GetIO().MousePos;
  ImVec2 localMouse = ImVec2(mousePos.x - g_PRCtx.CanvasPos.x, mousePos.y - g_PRCtx.CanvasPos.y);

  bool isHovered = (localMouse.x >= x1 && localMouse.x <= x2 && localMouse.y >= y1 && localMouse.y <= y2);
  auto const resize_zone_size = std::max(0.25f * (x2 - x1), 8.0f);
  bool isResizeZone = !isPercussive && isHovered && (x2 - localMouse.x <= resize_zone_size);

  if (state.activeNoteId == -999 && isHovered) {
    state.activeNoteId = id;
  }

  bool isActive = (state.activeNoteId == (int64_t)id);

  if (isHovered) resultFlags |= PianoRollInteraction_Hovered;

  if (isHovered && IsMouseClicked(ImGuiMouseButton_Left) && state.editMode == PianoRollEditMode::None) {
    state.activeNoteId = id;
    state.dragStartBeat = *beat;
    state.dragStartPitch = *pitch;
    state.dragStartVelocity = *velocity;
    state.dragStartDuration = *duration;

    if (io.KeyCtrl) {
      state.editMode = PianoRollEditMode::Velocity;
      state.mouseClick = ImVec2(localMouse.x, localMouse.y);
    } else {
      if (isResizeZone) {
        state.editMode = PianoRollEditMode::Resizing;
      } else {
        state.editMode = PianoRollEditMode::Moving;
        if (isPercussive) {
          state.mouseClickOffset = ImVec2(localMouse.x - centerX, localMouse.y - y1);
        } else {
          state.mouseClickOffset = ImVec2(localMouse.x - x1, localMouse.y - y1);
        }
      }
    }
    resultFlags |= PianoRollInteraction_Clicked;
  }

  if (isHovered && IsMouseClicked(ImGuiMouseButton_Right)) {
    resultFlags |= PianoRollInteraction_RightClicked;
  }

  if (isActive && IsMouseDown(ImGuiMouseButton_Left)) {
    if (state.editMode == PianoRollEditMode::Moving) {
      float targetX = localMouse.x - state.mouseClickOffset.x;
      float targetY = localMouse.y - state.mouseClickOffset.y;

      double newBeat = CanvasXToBeat(targetX, state.pixelsPerBeat, state.scrollVal.x);
      int newPitch = CanvasYToPitch(targetY + (state.noteHeight / 2.0f), state.noteHeight, state.scrollVal.y);
      if (state.snap_to_grid) {
        const auto sub_beat = 1.0 / state.sub_beat_grid;
        newBeat = std::round(newBeat / sub_beat) * sub_beat;
      }
      newBeat = ImMax(0.0, newBeat);

      if (*beat != newBeat) {
        *beat = newBeat;
        resultFlags |= PianoRollInteraction_PositionChanged;
      }
      if (*pitch != newPitch) {
        *pitch = newPitch;
        resultFlags |= PianoRollInteraction_PitchChanged;
      }
    } else if (state.editMode == PianoRollEditMode::Resizing) {
      double newDuration = CanvasXToBeat(localMouse.x, state.pixelsPerBeat, state.scrollVal.x) - *beat;
      const auto sub_beat = 1.0 / state.sub_beat_grid;
      if (state.snap_to_grid) {
        newDuration = std::round(newDuration / sub_beat) * sub_beat;
      }
      newDuration = ImMax(sub_beat, newDuration);

      if (*duration != newDuration) {
        *duration = newDuration;
        resultFlags |= PianoRollInteraction_DurationChanged;
      }
    } else if (state.editMode == PianoRollEditMode::Velocity) {
      float const delta_velo = -0.5f * (localMouse.y - state.mouseClick.y);
      int const newVelocity = std::max(0, std::min(127, static_cast<int>(state.dragStartVelocity + delta_velo)));
      if (*velocity != newVelocity) {
        *velocity = newVelocity;
        resultFlags |= PianoRollInteraction_VelocityChanged;
      }
    }
  }

  if (isHovered && io.MouseWheel != 0.0f && !io.KeyCtrl) {
    int const newVelocity = std::max(0, std::min(127, static_cast<int>(*velocity + io.MouseWheel)));
    if (*velocity != newVelocity) {
      *velocity = newVelocity;
      resultFlags |= PianoRollInteraction_VelocityChanged;
    }
  }

  if (isActive || isHovered) {
    if (state.editMode == PianoRollEditMode::Resizing || isResizeZone) {
      SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    } else {
      SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }
  }

  auto const finalColor = f_color ? f_color(isActive, isHovered, isResizeZone, *velocity)
                                  : PianoRollDefaultNoteColor(isActive, isHovered, isResizeZone, *velocity);

  if (isPercussive) {
    ImVec2 points[4] = {ImVec2(g_PRCtx.CanvasPos.x + centerX, g_PRCtx.CanvasPos.y + centerY - diamondRadius),
                        ImVec2(g_PRCtx.CanvasPos.x + centerX + diamondRadius, g_PRCtx.CanvasPos.y + centerY),
                        ImVec2(g_PRCtx.CanvasPos.x + centerX, g_PRCtx.CanvasPos.y + centerY + diamondRadius),
                        ImVec2(g_PRCtx.CanvasPos.x + centerX - diamondRadius, g_PRCtx.CanvasPos.y + centerY)};
    g_PRCtx.DrawList->AddConvexPolyFilled(points, 4, finalColor);
    g_PRCtx.DrawList->AddPolyline(points, 4, IM_COL32(0, 0, 0, 200), ImDrawFlags_Closed, 1.5f);
  } else {
    ImVec2 p_min = ImVec2(g_PRCtx.CanvasPos.x + x1, g_PRCtx.CanvasPos.y + y1);
    ImVec2 p_max = ImVec2(g_PRCtx.CanvasPos.x + x2, g_PRCtx.CanvasPos.y + y2);

    g_PRCtx.DrawList->AddRectFilled(p_min, p_max, finalColor, 3.0f);
    g_PRCtx.DrawList->AddRect(p_min, p_max, IM_COL32(0, 0, 0, 150), 3.0f, 0, 1.0f);
  }

  if (isHovered || isActive) {
    ImVec2 const text_pos(g_PRCtx.CanvasPos.x + x2 + 3.0f, g_PRCtx.CanvasPos.y + y1);
    auto const text_color = ImGui::GetColorU32(ImGuiCol_Text);
    std::string const velocity_text = std::to_string(*velocity);
    g_PRCtx.DrawList->AddText(text_pos, text_color, velocity_text.c_str());
  }

  return resultFlags;
}

void EndPianoRoll(PianoRollState& state) {
  if (IsMouseReleased(ImGuiMouseButton_Left)) {
    state.editMode = PianoRollEditMode::None;
    state.activeNoteId = -1;
  }

  g_PRCtx.DrawList->PopClipRect();

  DrawPianoKeysOverlay(g_PRCtx.DrawList, g_PRCtx.CanvasPos, g_PRCtx.CanvasSize, state.noteHeight, state.scrollVal);

  EndChild();
  PopID();
}

}  // namespace ImGui
