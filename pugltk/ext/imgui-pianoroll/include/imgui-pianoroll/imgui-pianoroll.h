#pragma once

#include <cstdint>
#include <chrono>
#include <functional>
#include <optional>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "imgui.h"

// Width of the piano keyboard overlay, in canvas pixels.
inline constexpr float PIANO_KEY_OVERLAY_WIDTH = 40.0f;

// Interaktions-Ergebnisse für einzelne Noten
enum PianoRollInteractionFlags_ {
  PianoRollInteraction_None = 0,
  PianoRollInteraction_Hovered = 1 << 0,
  PianoRollInteraction_Clicked = 1 << 1,
  PianoRollInteraction_RightClicked = 1 << 2,
  PianoRollInteraction_PositionChanged = 1 << 3,
  PianoRollInteraction_DurationChanged = 1 << 4,
  PianoRollInteraction_VelocityChanged = 1 << 5,
  PianoRollInteraction_PitchChanged = 1 << 6
};
typedef int PianoRollInteraction;

// Interner Bearbeitungsmodus
enum class PianoRollEditMode { None, Moving, Resizing, Velocity };

enum class PianoRollKeyboardEventType { NoteOn, NoteOff };
using PianoRollKeyboardEventCallback = std::function<void(PianoRollKeyboardEventType, int pitch)>;
using PianoRollVelocityCommitCallback = std::function<void(uint32_t note_id, int velocity)>;
// Invoked once per selected note when DEL is pressed; the client removes the note.
using PianoRollDeleteCallback = std::function<void(uint32_t note_id)>;

// Kapselt den vollständigen UI-Zustand eines Piano Rolls
struct PianoRollState {
  ImVec2 scrollVal = ImVec2(0.0f, 1000.0f);  // Startet vertikal leicht mittig
  float pixelsPerBeat = 100.0f;              // Horizontaler Zoom
  float noteHeight = 20.0f;                  // Vertikaler Zoom
  int sub_beat_grid = 4;                     // 16th note by default
  int beats_per_bar = 4;
  bool snap_to_grid = true;
  bool percussive_notes = false;
  bool show_velocity_editor = false;
  std::optional<int> velocity_filter_key;
  // Invoked once per debounced edit; the client applies or sends the change.
  PianoRollVelocityCommitCallback velocity_commit;
  // Invoked for every selected note when DEL is pressed over the canvas. Without it DEL is ignored by the widget.
  PianoRollDeleteCallback delete_note;

  // --- Note selection (group edit) ---
  // Selected note ids. Filled by right-click rectangle selection or right-click on the overlay keyboard
  // (all notes of that pitch). Shift adds to the selection. Escape or a left click on empty space clears it.
  // Dragging a selected note moves the whole group; velocity edits of one member offset all members.
  // Ids of notes not submitted via PianoRollNote in a frame are dropped from the selection.
  std::set<uint32_t> selected_notes;
  struct SelectedNoteOrigin {
    double beat;
    int pitch;
    int velocity;
  };
  std::unordered_map<uint32_t, SelectedNoteOrigin> selection_origins;  // Pre-edit values of the selected notes.
  bool group_dragging = false;
  bool rect_selecting = false;
  ImVec2 rect_start_world = ImVec2(0.0f, 0.0f);  // Scroll-independent start of the selection rectangle.
  std::set<uint32_t> rect_base_selection;
  bool left_click_on_empty = false;

  struct PendingVelocity {
    uint32_t note_id;
    int original_velocity;
    int velocity;
    std::chrono::steady_clock::time_point last_update;
    // Other selected notes (id, original velocity) that receive the same velocity offset.
    std::vector<std::pair<uint32_t, int>> group;
  };
  std::optional<PendingVelocity> pending_velocity;
  bool velocity_dragging = false;
  bool velocity_rendered = false;

  // Fit-Zielwerte für die eingebetteten Zoom-Buttons.
  int fitMinPitch = 48;
  int fitMaxPitch = 84;
  double fitTotalBeats = 8.0;
  double fitPatternLengthBeats = 8.0;

  // Drag- & Interaktionszustand
  PianoRollEditMode editMode = PianoRollEditMode::None;
  int64_t activeNoteId = -1;
  double dragStartBeat = 0.0;
  int dragStartPitch = 0;
  int dragStartVelocity = 0;
  double dragStartDuration = 0.0;
  ImVec2 mouseClick = ImVec2(0.0f, 0.0f);
  ImVec2 mouseClickOffset = ImVec2(0.0f, 0.0f);
  int keyboardActivePitch = -1;

  static PianoRollState CreateDefaultMelodic() {
    PianoRollState state;
    state.percussive_notes = false;
    return state;
  }

  static PianoRollState CreateDefaultPercussive() {
    PianoRollState state;
    state.percussive_notes = true;
    return state;
  }
};

enum PianoRollControlFlags_ {
  PianoRollControl_None = 0,
  PianoRollControl_ShowPercussiveSwitch = 1 << 0,
  PianoRollControl_ShowSnapToGridSwitch = 1 << 1,
  PianoRollControl_ShowBeatsPerBarSetting = 1 << 2,
  PianoRollControl_ShowSubBeatGridSetting = 1 << 3,
  PianoRollControl_ShowFitControls = 1 << 4,
  PianoRollControl_ShowVelocityEditorSwitch = 1 << 5,  // Requires velocity_commit and an ImPlot context.
};
typedef int PianoRollControl;

static auto constexpr PianoRollControl_DefaultFlags =
    PianoRollControl_ShowPercussiveSwitch | PianoRollControl_ShowSnapToGridSwitch |
    PianoRollControl_ShowBeatsPerBarSetting | PianoRollControl_ShowSubBeatGridSetting;

namespace ImGui {
// A callback enables debounced edits; without one PianoRollNote retains immediate pointer updates.
// Call UpdatePianoRollVelocity once per frame, even when hidden, and force-flush before closing.
// Cancel pending edits when note IDs are removed, reused, or remapped.
void UpdatePianoRollVelocity(PianoRollState& state);
bool FlushPianoRollVelocity(PianoRollState& state, bool force = false,
                            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());
void CancelPianoRollVelocity(PianoRollState& state);
void QueuePianoRollVelocity(PianoRollState& state, uint32_t note_id, int original_velocity, int velocity,
                            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());
int PianoRollDisplayedVelocity(PianoRollState const& state, uint32_t note_id, int velocity);
// Optional named filter options; call between BeginPianoRoll and EndPianoRoll.
void PianoRollVelocityKey(int key_id, const char* label);
void FitPianoRollVertical(PianoRollState& state, int minPitch, int maxPitch, const ImVec2& canvasSize,
                         float contentPadding = 48.0f);
void FitPianoRollHorizontal(PianoRollState& state, double totalBeats, const ImVec2& canvasSize,
                           float contentPadding = 48.0f);
void FitPianoRollFullLength(PianoRollState& state, double fullLengthBeats, const ImVec2& canvasSize,
                           float contentPadding = 48.0f);
void FitPianoRollToContent(PianoRollState& state, int minPitch, int maxPitch, double totalBeats,
                           const ImVec2& canvasSize, float contentPadding = 48.0f);

// Startet den Canvas
// The optional callback receives overlay-key NoteOn/NoteOff events as (type, MIDI pitch, velocity).
bool BeginPianoRoll(const char* str_id, PianoRollState& state, const ImVec2& size = ImVec2(0, 0),
                    PianoRollControl flags = PianoRollControl_DefaultFlags,
                    PianoRollKeyboardEventCallback keyboardEventCallback = nullptr);

// Erstellt eine neue Note per Single-Click / Touch (inklusive Drag zum Aufziehen)
bool PianoRollCreateNote(PianoRollState& state, double* outBeat, int* outPitch, int* outVelocity, double* outDuration);

inline ImU32 constexpr PianoRollDefaultNoteColor(bool active, bool hovered_default, bool hovered_resize_zone,
                                                 int velocity) {
  if (active || hovered_resize_zone) return IM_COL32(127 + velocity, 255 - velocity, 127, 255);
  if (hovered_default) return IM_COL32(107 + velocity, 235 - velocity, 107, 255);
  return IM_COL32(87 + velocity, 215 - velocity, 87, 255);
}

// With a commit callback, velocity is previewed internally and only the callback commits it.
// velocity_key groups notes for filtering and colors; -1 uses the note's pitch.
PianoRollInteraction PianoRollNote(PianoRollState& state, uint32_t id, double* beat, int* pitch, int* velocity,
                                   double* duration, std::function<ImU32(bool, bool, bool, int)> f_color = nullptr,
                                   int velocity_key = -1);

// Schließt den Canvas und zeichnet Overlays
void EndPianoRoll(PianoRollState& state);

// Demo
void ShowPianoRollDemoApp();

}  // namespace ImGui
