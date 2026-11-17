#include "imgui-pianoroll/imgui-pianoroll.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "imgui.h"
#include "imgui_internal.h"
#include "implot.h"

// Interner Kontext für den aktuellen Frame
struct PianoRollVelocityNote {
  uint32_t id;
  int key;
  double beat;
  int original_velocity;
};

struct PianoRollNoteBounds {
  uint32_t id;
  float x1, y1, x2, y2;  // Canvas-local pixels.
};

struct PianoRollContext {
  ImVec2 CanvasPos;
  ImVec2 CanvasSize;
  ImDrawList* DrawList = nullptr;
  bool IsCanvasHovered = false;
  bool VelocityPane = false;
  std::vector<PianoRollVelocityNote> Notes;
  std::vector<std::pair<int, std::string>> Keys;

  // Selection / group edit, valid for the current frame only.
  std::vector<PianoRollNoteBounds> Bounds;
  int SelectPitch = -1;           // Right click on overlay key: notes of this pitch join the selection.
  bool RectCandidate = false;     // Right click this frame; becomes a rectangle selection if no note was hit.
  ImVec2 RectCandidateWorld;
  bool RightClickedNote = false;
  bool LeftClickCandidate = false;
  bool GroupMoveActive = false;
  double GroupDeltaBeat = 0.0;
  int GroupDeltaPitch = 0;
};

static PianoRollContext g_PRCtx;
static constexpr float MIN_SCROLL_X = -2.0f * PIANO_KEY_OVERLAY_WIDTH;

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

    if (state.editMode == PianoRollEditMode::None && state.keyboardActivePitch == -1 &&
        ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
      state.scrollVal.x -= io.MouseDelta.x;
      state.scrollVal.y -= io.MouseDelta.y;
    }
  }

  state.scrollVal.x = ImMax(MIN_SCROLL_X, state.scrollVal.x);
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

static void HandlePianoKeyboardInput(PianoRollState& state, PianoRollKeyboardEventCallback const& callback) {
  ImGuiIO& io = ImGui::GetIO();
  ImVec2 const localMouse(io.MousePos.x - g_PRCtx.CanvasPos.x, io.MousePos.y - g_PRCtx.CanvasPos.y);
  bool const overKeyboard = g_PRCtx.IsCanvasHovered && localMouse.x >= 0.0f &&
                            localMouse.x < PIANO_KEY_OVERLAY_WIDTH && localMouse.y >= 0.0f &&
                            localMouse.y < g_PRCtx.CanvasSize.y;

  if (overKeyboard && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && state.keyboardActivePitch == -1) {
    state.keyboardActivePitch = ImClamp(CanvasYToPitch(localMouse.y, state.noteHeight, state.scrollVal.y), 0, 127);
    if (callback) {
      callback(PianoRollKeyboardEventType::NoteOn, state.keyboardActivePitch);
    }
  }

  if (state.keyboardActivePitch != -1 && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    if (callback) {
      callback(PianoRollKeyboardEventType::NoteOff, state.keyboardActivePitch);
    }
    state.keyboardActivePitch = -1;
  }

  if (overKeyboard && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
    if (!io.KeyShift) {
      state.selected_notes.clear();
    }
    g_PRCtx.SelectPitch = ImClamp(CanvasYToPitch(localMouse.y, state.noteHeight, state.scrollVal.y), 0, 127);
  }
}

// Prepares rectangle selection and selection clearing; the decisions are finished in EndPianoRoll, once it is
// known whether a note was hit.
static void HandleSelectionInput(PianoRollState& state) {
  ImGuiIO& io = ImGui::GetIO();
  ImVec2 const localMouse(io.MousePos.x - g_PRCtx.CanvasPos.x, io.MousePos.y - g_PRCtx.CanvasPos.y);
  bool const overNotes = g_PRCtx.IsCanvasHovered && localMouse.x >= PIANO_KEY_OVERLAY_WIDTH && localMouse.y >= 0.0f &&
                         localMouse.y < g_PRCtx.CanvasSize.y;

  if (overNotes && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !state.rect_selecting) {
    g_PRCtx.RectCandidate = true;
    g_PRCtx.RectCandidateWorld = ImVec2(localMouse.x + state.scrollVal.x, localMouse.y + state.scrollVal.y);
  }
  if (overNotes && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && state.editMode == PianoRollEditMode::None &&
      state.keyboardActivePitch == -1) {
    g_PRCtx.LeftClickCandidate = true;
  }
  if (g_PRCtx.IsCanvasHovered && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
    state.selected_notes.clear();
  }
}

// Computes the group offset from the current mouse position. The leftmost selected note defines the beat snap.
static void UpdateGroupMove(PianoRollState& state) {
  g_PRCtx.GroupMoveActive = false;
  if (!state.group_dragging || state.editMode != PianoRollEditMode::Moving ||
      !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    return;
  }
  double min_beat = 0.0;
  int min_pitch = 127, max_pitch = 0;
  bool any = false;
  for (auto const id : state.selected_notes) {
    auto const it = state.selection_origins.find(id);
    if (it == state.selection_origins.end()) continue;
    min_beat = any ? std::min(min_beat, it->second.beat) : it->second.beat;
    min_pitch = std::min(min_pitch, it->second.pitch);
    max_pitch = std::max(max_pitch, it->second.pitch);
    any = true;
  }
  if (!any) return;

  ImGuiIO& io = ImGui::GetIO();
  float const target_x = io.MousePos.x - g_PRCtx.CanvasPos.x - state.mouseClickOffset.x;
  float const target_y = io.MousePos.y - g_PRCtx.CanvasPos.y - state.mouseClickOffset.y;
  double const raw_beat = CanvasXToBeat(target_x, state.pixelsPerBeat, state.scrollVal.x);
  int const raw_pitch = CanvasYToPitch(target_y + state.noteHeight / 2.0f, state.noteHeight, state.scrollVal.y);

  double new_leftmost = min_beat + (raw_beat - state.dragStartBeat);
  if (state.snap_to_grid) {
    new_leftmost = std::round(new_leftmost * state.sub_beat_grid) / state.sub_beat_grid;
  }
  new_leftmost = std::max(0.0, new_leftmost);

  g_PRCtx.GroupDeltaBeat = new_leftmost - min_beat;
  g_PRCtx.GroupDeltaPitch = std::clamp(raw_pitch - state.dragStartPitch, -min_pitch, 127 - max_pitch);
  g_PRCtx.GroupMoveActive = true;
}

namespace ImGui {

void UpdatePianoRollVelocity(PianoRollState& state) {
  if (!state.velocity_rendered) state.velocity_dragging = false;
  FlushPianoRollVelocity(state);
  state.velocity_rendered = false;
}

bool FlushPianoRollVelocity(PianoRollState& state, bool force, std::chrono::steady_clock::time_point now) {
  if (!state.pending_velocity || (!force && (state.velocity_dragging ||
      now - state.pending_velocity->last_update < std::chrono::milliseconds(200)))) {
    return false;
  }
  auto const change = *state.pending_velocity;
  state.pending_velocity.reset();
  if (change.velocity == change.original_velocity) return false;
  IM_ASSERT(state.velocity_commit && "Piano roll velocity edits require a commit callback");
  if (!state.velocity_commit) return false;
  state.velocity_commit(change.note_id, change.velocity);
  int const delta = change.velocity - change.original_velocity;
  for (auto const& [id, original] : change.group) {
    int const velocity = std::clamp(original + delta, 0, 127);
    if (velocity != original) state.velocity_commit(id, velocity);
  }
  return true;
}

void CancelPianoRollVelocity(PianoRollState& state) {
  state.pending_velocity.reset();
  state.velocity_dragging = false;
}

void QueuePianoRollVelocity(PianoRollState& state, uint32_t note_id, int original_velocity, int velocity,
                            std::chrono::steady_clock::time_point now) {
  IM_ASSERT(state.velocity_commit && "Piano roll velocity edits require a commit callback");
  if (!state.velocity_commit) return;
  if (state.pending_velocity && state.pending_velocity->note_id != note_id) {
    FlushPianoRollVelocity(state, true, now);
  }
  std::vector<std::pair<uint32_t, int>> group;
  if (state.pending_velocity) {
    original_velocity = state.pending_velocity->original_velocity;
    group = std::move(state.pending_velocity->group);
  } else if (state.selected_notes.count(note_id)) {
    // The edited note belongs to the selection: all other members receive the same offset.
    for (auto const id : state.selected_notes) {
      auto const origin = state.selection_origins.find(id);
      if (id != note_id && origin != state.selection_origins.end()) {
        group.emplace_back(id, origin->second.velocity);
      }
    }
  }
  state.pending_velocity =
      PianoRollState::PendingVelocity{note_id, original_velocity, std::clamp(velocity, 0, 127), now, std::move(group)};
}

int PianoRollDisplayedVelocity(PianoRollState const& state, uint32_t note_id, int velocity) {
  if (!state.pending_velocity) return velocity;
  auto const& pending = *state.pending_velocity;
  if (pending.note_id == note_id) return pending.velocity;
  for (auto const& [id, original] : pending.group) {
    if (id == note_id) return std::clamp(original + (pending.velocity - pending.original_velocity), 0, 127);
  }
  return velocity;
}

void PianoRollVelocityKey(int key_id, const char* label) {
  g_PRCtx.Keys.emplace_back(key_id, label);
}

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

  constexpr float kHorizontalZoomOutFactor = 1.1f;
  auto const visibleWidth = std::max(1.0f, canvasSize.x - contentPadding) + MIN_SCROLL_X; // MIN_SCROLL_X is negative
  auto const durationBeats = std::max(1.0, totalBeats * kHorizontalZoomOutFactor);
  state.pixelsPerBeat = std::clamp(visibleWidth / static_cast<float>(durationBeats), 20.0f, 800.0f);
  state.scrollVal.x = MIN_SCROLL_X;
}

void FitPianoRollFullLength(PianoRollState& state, double fullLengthBeats, const ImVec2& canvasSize,
                           float contentPadding) {
  if (canvasSize.x <= 0.0f || canvasSize.y <= 0.0f) {
    return;
  }

  constexpr float kHorizontalZoomOutFactor = 1.1f;
  auto const visibleWidth = std::max(1.0f, canvasSize.x - contentPadding) + MIN_SCROLL_X; // MIN_SCROLL_X is negative
  auto const durationBeats = std::max(1.0, fullLengthBeats * kHorizontalZoomOutFactor);
  state.pixelsPerBeat = std::clamp(visibleWidth / static_cast<float>(durationBeats), 20.0f, 800.0f);
  state.scrollVal.x = MIN_SCROLL_X;
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

bool BeginPianoRoll(const char* str_id, PianoRollState& state, const ImVec2& size, PianoRollControl flags,
                    PianoRollKeyboardEventCallback keyboardEventCallback) {
  FlushPianoRollVelocity(state);
  state.velocity_dragging = false;
  state.velocity_rendered = true;
  g_PRCtx.Notes.clear();
  g_PRCtx.Keys.clear();
  g_PRCtx.Bounds.clear();
  g_PRCtx.SelectPitch = -1;
  g_PRCtx.RectCandidate = false;
  g_PRCtx.RightClickedNote = false;
  g_PRCtx.LeftClickCandidate = false;
  g_PRCtx.GroupMoveActive = false;
  g_PRCtx.VelocityPane = false;
  PushID(str_id);

  BeginChild("PianoRollCanvas", size, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);

  auto const available_size = GetContentRegionAvail().x;
  auto const widget_size = std::min(100.0f, available_size / 4.0f);

  bool const any_control_visible =
      (flags & PianoRollControl_ShowPercussiveSwitch) || (flags & PianoRollControl_ShowSnapToGridSwitch) ||
      (flags & PianoRollControl_ShowBeatsPerBarSetting) || (flags & PianoRollControl_ShowSubBeatGridSetting) ||
      (flags & PianoRollControl_ShowFitControls) || (flags & PianoRollControl_ShowVelocityEditorSwitch);

  bool velocity_toggled = false;
  if (flags & PianoRollControl_ShowVelocityEditorSwitch) {
    velocity_toggled = ImGui::Checkbox("velocities", &state.show_velocity_editor);
    ImGui::SameLine();
  }

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

  g_PRCtx.VelocityPane = (flags & PianoRollControl_ShowVelocityEditorSwitch) && state.show_velocity_editor;
  if (g_PRCtx.VelocityPane) {
    IM_ASSERT(state.velocity_commit && "Velocity editor requires a commit callback");
    float const half_height = std::max(1.0f, (GetContentRegionAvail().y - GetStyle().ItemSpacing.y) * 0.5f);
    BeginChild("PianoRollNotes", ImVec2(0, half_height), true,
               ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
  }
  g_PRCtx.CanvasPos = GetCursorScreenPos();
  g_PRCtx.CanvasSize = GetContentRegionAvail();
  g_PRCtx.DrawList = GetWindowDrawList();
  g_PRCtx.IsCanvasHovered = IsWindowHovered();

  if (velocity_toggled) {
    FitPianoRollVertical(state, state.fitMinPitch, state.fitMaxPitch, g_PRCtx.CanvasSize);
  }
  HandleCanvasInput(state, g_PRCtx.CanvasSize);
  HandlePianoKeyboardInput(state, keyboardEventCallback);
  HandleSelectionInput(state);
  UpdateGroupMove(state);
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

    clickBeat = ImMax(0.0, std::round(clickBeat * state.sub_beat_grid) / state.sub_beat_grid);
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
                                   double* duration, std::function<ImU32(bool, bool, bool, int)> f_color,
                                   int velocity_key) {
  PianoRollInteraction resultFlags = PianoRollInteraction_None;
  int const original_velocity = *velocity;
  int preview_velocity = PianoRollDisplayedVelocity(state, id, *velocity);
  if (state.velocity_commit) velocity = &preview_velocity;

  if (g_PRCtx.SelectPitch == *pitch) {
    state.selected_notes.insert(id);
  }
  if (state.selected_notes.count(id)) {
    // Remember pre-edit values so group moves and group velocity offsets stay relative to the drag start.
    auto const [origin, inserted] =
        state.selection_origins.try_emplace(id, PianoRollState::SelectedNoteOrigin{*beat, *pitch, original_velocity});
    if (!inserted) {
      if (!state.group_dragging) {
        origin->second.beat = *beat;
        origin->second.pitch = *pitch;
      }
      if (!state.pending_velocity) origin->second.velocity = original_velocity;
    }
    if (g_PRCtx.GroupMoveActive) {
      double const new_beat = origin->second.beat + g_PRCtx.GroupDeltaBeat;
      int const new_pitch = origin->second.pitch + g_PRCtx.GroupDeltaPitch;
      if (std::abs(*beat - new_beat) > 1e-9) {
        *beat = new_beat;
        resultFlags |= PianoRollInteraction_PositionChanged;
      }
      if (*pitch != new_pitch) {
        *pitch = new_pitch;
        resultFlags |= PianoRollInteraction_PitchChanged;
      }
    }
  }

  if (g_PRCtx.VelocityPane) {
    g_PRCtx.Notes.push_back({id, velocity_key < 0 ? *pitch : velocity_key, *beat, original_velocity});
  }
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

  g_PRCtx.Bounds.push_back({id, x1, y1, x2, y2});

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

    if (!state.selected_notes.count(id)) {
      // Clicking a note outside the selection drops the selection and edits only this note.
      state.selected_notes.clear();
    }

    if (io.KeyCtrl) {
      state.editMode = PianoRollEditMode::Velocity;
      state.mouseClick = ImVec2(localMouse.x, localMouse.y);
    } else {
      if (isResizeZone) {
        state.editMode = PianoRollEditMode::Resizing;
      } else {
        state.editMode = PianoRollEditMode::Moving;
        state.group_dragging = state.selected_notes.size() > 1;
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
    g_PRCtx.RightClickedNote = true;
  }

  if (isActive && IsMouseDown(ImGuiMouseButton_Left)) {
    if (state.editMode == PianoRollEditMode::Moving && !state.group_dragging) {
      float targetX = localMouse.x - state.mouseClickOffset.x;
      float targetY = localMouse.y - state.mouseClickOffset.y;

      double newBeat = CanvasXToBeat(targetX, state.pixelsPerBeat, state.scrollVal.x);
      int newPitch = CanvasYToPitch(targetY + (state.noteHeight / 2.0f), state.noteHeight, state.scrollVal.y);
      if (state.snap_to_grid) {
        newBeat = std::round(newBeat * state.sub_beat_grid) / state.sub_beat_grid;
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
      if (state.snap_to_grid) {
        newDuration = std::round(newDuration * state.sub_beat_grid) / state.sub_beat_grid;
      }
      newDuration = ImMax(1.0 / state.sub_beat_grid, newDuration);

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

  if (state.velocity_commit && (resultFlags & PianoRollInteraction_VelocityChanged)) {
    QueuePianoRollVelocity(state, id, original_velocity, *velocity);
  }
  if (state.velocity_commit && state.editMode == PianoRollEditMode::Velocity && state.activeNoteId == id) {
    state.velocity_dragging = IsMouseDown(ImGuiMouseButton_Left);
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
    if (state.selected_notes.count(id)) {
      float const r = diamondRadius + 3.0f;
      ImVec2 const frame[4] = {ImVec2(g_PRCtx.CanvasPos.x + centerX, g_PRCtx.CanvasPos.y + centerY - r),
                               ImVec2(g_PRCtx.CanvasPos.x + centerX + r, g_PRCtx.CanvasPos.y + centerY),
                               ImVec2(g_PRCtx.CanvasPos.x + centerX, g_PRCtx.CanvasPos.y + centerY + r),
                               ImVec2(g_PRCtx.CanvasPos.x + centerX - r, g_PRCtx.CanvasPos.y + centerY)};
      g_PRCtx.DrawList->AddPolyline(frame, 4, IM_COL32(255, 255, 255, 255), ImDrawFlags_Closed, 2.0f);
    }
  } else {
    ImVec2 p_min = ImVec2(g_PRCtx.CanvasPos.x + x1, g_PRCtx.CanvasPos.y + y1);
    ImVec2 p_max = ImVec2(g_PRCtx.CanvasPos.x + x2, g_PRCtx.CanvasPos.y + y2);

    g_PRCtx.DrawList->AddRectFilled(p_min, p_max, finalColor, 3.0f);
    g_PRCtx.DrawList->AddRect(p_min, p_max, IM_COL32(0, 0, 0, 150), 3.0f, 0, 1.0f);
    if (state.selected_notes.count(id)) {
      g_PRCtx.DrawList->AddRect(ImVec2(p_min.x - 2.0f, p_min.y - 2.0f), ImVec2(p_max.x + 2.0f, p_max.y + 2.0f),
                                IM_COL32(255, 255, 255, 255), 4.0f, 0, 2.0f);
    }
  }

  if (isHovered || isActive) {
    ImVec2 const text_pos(g_PRCtx.CanvasPos.x + x2 + 3.0f, g_PRCtx.CanvasPos.y + y1);
    auto const text_color = ImGui::GetColorU32(ImGuiCol_Text);
    std::string const velocity_text = std::to_string(*velocity);
    g_PRCtx.DrawList->AddText(text_pos, text_color, velocity_text.c_str());
  }

  return resultFlags;
}

static void RenderPianoRollVelocities(PianoRollState& state) {
  for (auto const& note : g_PRCtx.Notes) {
    auto const found = std::find_if(g_PRCtx.Keys.begin(), g_PRCtx.Keys.end(),
                                    [&](auto const& key) { return key.first == note.key; });
    if (found == g_PRCtx.Keys.end()) {
      g_PRCtx.Keys.emplace_back(note.key, "Key " + std::to_string(note.key));
    }
  }
  auto selected = std::find_if(g_PRCtx.Keys.begin(), g_PRCtx.Keys.end(),
                               [&](auto const& key) { return state.velocity_filter_key == key.first; });
  if (state.velocity_filter_key && selected == g_PRCtx.Keys.end()) state.velocity_filter_key.reset();
  BeginChild("PianoRollVelocities", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);
  SetNextItemWidth(250.0f);
  if (BeginCombo("Key filter", state.velocity_filter_key ? selected->second.c_str() : "All keys")) {
    if (Selectable("All keys", !state.velocity_filter_key)) state.velocity_filter_key.reset();
    for (auto const& key : g_PRCtx.Keys) {
      PushID(key.first);
      if (Selectable(key.second.c_str(), state.velocity_filter_key == key.first)) state.velocity_filter_key = key.first;
      PopID();
    }
    EndCombo();
  }
  SameLine();
  TextUnformatted("Velocity (0-127)");

  auto const plot_x = g_PRCtx.CanvasPos.x + PIANO_KEY_OVERLAY_WIDTH;
  auto const plot_width = g_PRCtx.CanvasSize.x - PIANO_KEY_OVERLAY_WIDTH;
  if (plot_width > 0) {
    SetCursorScreenPos(ImVec2(plot_x, GetCursorScreenPos().y));
    ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
    if (ImPlot::BeginPlot("##Velocities", ImVec2(plot_width, std::max(1.0f, GetContentRegionAvail().y)),
                          ImPlotFlags_CanvasOnly | ImPlotFlags_NoInputs)) {
      ImPlot::SetupAxis(ImAxis_X1, "Beat");
      ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks);
      ImPlot::SetupAxisLimits(ImAxis_X1, (PIANO_KEY_OVERLAY_WIDTH + state.scrollVal.x) / state.pixelsPerBeat,
                              (g_PRCtx.CanvasSize.x + state.scrollVal.x) / state.pixelsPerBeat, ImPlotCond_Always);
      ImPlot::SetupAxisLimits(ImAxis_Y1, -5, 132, ImPlotCond_Always);
      ImPlot::SetupFinish();
      for (int value : {0, 64, 127}) {
        auto const pos = ImPlot::PlotToPixels(0.0, value);
        auto const label = std::to_string(value);
        GetWindowDrawList()->AddText(ImVec2(plot_x - CalcTextSize(label.c_str()).x - 5.0f,
                                             pos.y - GetTextLineHeight() * 0.5f),
                                     GetColorU32(ImGuiCol_Text), label.c_str());
      }

      auto& notes = g_PRCtx.Notes;
      notes.erase(std::remove_if(notes.begin(), notes.end(), [&](auto const& note) {
                    return state.velocity_filter_key && note.key != *state.velocity_filter_key;
                  }), notes.end());
      std::sort(notes.begin(), notes.end(), [](auto const& a, auto const& b) {
        return a.beat != b.beat ? a.beat < b.beat : a.id < b.id;
      });
      size_t start = 0;
      while (start < notes.size()) {
        size_t end = start + 1;
        while (end < notes.size() && notes[end].beat == notes[start].beat) ++end;
        for (size_t i = start; i < end; ++i) {
          auto const& note = notes[i];
          // Separate simultaneous handles without changing the note's beat.
          double x = note.beat + (static_cast<double>(i - start) - (end - start - 1) * 0.5) *
                                      10.0 / state.pixelsPerBeat;
          auto const x_pixel = g_PRCtx.CanvasPos.x + BeatToCanvasX(x, state.pixelsPerBeat, state.scrollVal.x);
          if (x_pixel < plot_x - 4.0f || x_pixel > plot_x + plot_width + 4.0f) continue;
          double y = PianoRollDisplayedVelocity(state, note.id, note.original_velocity);
          float const hue = std::fmod(static_cast<float>(note.key) * 0.618034f, 1.0f);
          ImVec4 color;
          ColorConvertHSVtoRGB(hue, 0.65f, 0.9f, color.x, color.y, color.z);
          color.w = 1.0f;
          auto const label = "##velocity" + std::to_string(note.id);
          ImPlot::SetNextFillStyle(color);
          if (state.selected_notes.count(note.id)) {
            ImPlot::SetNextLineStyle(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 2.0f);
          } else {
            ImPlot::SetNextLineStyle(color);
          }
          ImPlot::PlotBars(label.c_str(), &x, &y, 1, 7.0 / state.pixelsPerBeat);
          bool hovered = false, held = false;
          if (ImPlot::DragPoint(static_cast<int>(note.id), &x, &y, color, 4.0f, ImPlotDragToolFlags_Delayed,
                                nullptr, &hovered, &held)) {
            QueuePianoRollVelocity(state, note.id, note.original_velocity,
                                   static_cast<int>(std::lround(std::clamp(y, 0.0, 127.0))));
          }
          state.velocity_dragging |= held;
          if (hovered || held) {
            auto const key = std::find_if(g_PRCtx.Keys.begin(), g_PRCtx.Keys.end(),
                                           [&](auto const& entry) { return entry.first == note.key; });
            SetTooltip("%s\nBeat: %.3f\nVelocity: %d", key->second.c_str(), note.beat,
                       PianoRollDisplayedVelocity(state, note.id, note.original_velocity));
          }
        }
        start = end;
      }
      ImPlot::EndPlot();
    }
    ImPlot::PopStyleVar();
  }
  EndChild();
}

void EndPianoRoll(PianoRollState& state) {
  ImGuiIO& io = GetIO();

  // Rectangle selection starts on a right click that did not hit a note.
  if (g_PRCtx.RectCandidate && !g_PRCtx.RightClickedNote) {
    state.rect_selecting = true;
    state.rect_start_world = g_PRCtx.RectCandidateWorld;
    state.rect_base_selection = io.KeyShift ? state.selected_notes : std::set<uint32_t>{};
    if (!io.KeyShift) state.selected_notes.clear();
  }
  if (g_PRCtx.LeftClickCandidate && state.editMode == PianoRollEditMode::None) {
    state.left_click_on_empty = true;
  }

  if (state.rect_selecting) {
    ImVec2 const start(state.rect_start_world.x - state.scrollVal.x, state.rect_start_world.y - state.scrollVal.y);
    ImVec2 const current(io.MousePos.x - g_PRCtx.CanvasPos.x, io.MousePos.y - g_PRCtx.CanvasPos.y);
    ImVec2 const rect_min(std::max(std::min(start.x, current.x), PIANO_KEY_OVERLAY_WIDTH), std::min(start.y, current.y));
    ImVec2 const rect_max(std::max(start.x, current.x), std::max(start.y, current.y));
    state.selected_notes = state.rect_base_selection;
    for (auto const& b : g_PRCtx.Bounds) {
      if (b.x2 >= rect_min.x && b.x1 <= rect_max.x && b.y2 >= rect_min.y && b.y1 <= rect_max.y) {
        state.selected_notes.insert(b.id);
      }
    }
    if (IsMouseDown(ImGuiMouseButton_Right)) {
      ImVec2 const p_min(g_PRCtx.CanvasPos.x + rect_min.x, g_PRCtx.CanvasPos.y + rect_min.y);
      ImVec2 const p_max(g_PRCtx.CanvasPos.x + rect_max.x, g_PRCtx.CanvasPos.y + rect_max.y);
      g_PRCtx.DrawList->AddRectFilled(p_min, p_max, IM_COL32(255, 255, 255, 25));
      g_PRCtx.DrawList->AddRect(p_min, p_max, IM_COL32(255, 255, 255, 180));
    } else {
      state.rect_selecting = false;
    }
  }

  // Drop selected notes that are no longer submitted (e.g. removed by the client).
  if (!state.selected_notes.empty() || !state.selection_origins.empty()) {
    std::vector<uint32_t> ids;
    ids.reserve(g_PRCtx.Bounds.size());
    for (auto const& b : g_PRCtx.Bounds) ids.push_back(b.id);
    std::sort(ids.begin(), ids.end());
    for (auto it = state.selected_notes.begin(); it != state.selected_notes.end();) {
      it = std::binary_search(ids.begin(), ids.end(), *it) ? std::next(it) : state.selected_notes.erase(it);
    }
    for (auto it = state.selection_origins.begin(); it != state.selection_origins.end();) {
      it = state.selected_notes.count(it->first) ? std::next(it) : state.selection_origins.erase(it);
    }
  }

  if (!state.selected_notes.empty() && state.delete_note && g_PRCtx.IsCanvasHovered && !io.WantTextInput &&
      IsKeyPressed(ImGuiKey_Delete, false)) {
    if (state.pending_velocity && state.selected_notes.count(state.pending_velocity->note_id)) {
      CancelPianoRollVelocity(state);
    } else {
      FlushPianoRollVelocity(state, true);
    }
    std::vector<uint32_t> const doomed(state.selected_notes.begin(), state.selected_notes.end());
    state.selected_notes.clear();
    state.selection_origins.clear();
    for (auto const id : doomed) state.delete_note(id);
  }

  if (IsMouseReleased(ImGuiMouseButton_Left)) {
    if (state.left_click_on_empty && !state.rect_selecting) {
      ImVec2 const moved(io.MousePos.x - io.MouseClickedPos[0].x, io.MousePos.y - io.MouseClickedPos[0].y);
      if (moved.x * moved.x + moved.y * moved.y < 16.0f) state.selected_notes.clear();
    }
    state.left_click_on_empty = false;
    state.editMode = PianoRollEditMode::None;
    state.activeNoteId = -1;
    state.group_dragging = false;
  }

  g_PRCtx.DrawList->PopClipRect();

  DrawPianoKeysOverlay(g_PRCtx.DrawList, g_PRCtx.CanvasPos, g_PRCtx.CanvasSize, state.noteHeight, state.scrollVal);

  if (g_PRCtx.VelocityPane) {
    EndChild();
    RenderPianoRollVelocities(state);
  }
  EndChild();
  PopID();
}

}  // namespace ImGui
