#pragma once

#include <cstdint>
#include <functional>

#include "imgui.h"

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

// Kapselt den vollständigen UI-Zustand eines Piano Rolls
struct PianoRollState {
  ImVec2 scrollVal = ImVec2(0.0f, 1000.0f);  // Startet vertikal leicht mittig
  float pixelsPerBeat = 100.0f;              // Horizontaler Zoom
  float noteHeight = 20.0f;                  // Vertikaler Zoom
  int sub_beat_grid = 4;                     // 16th note by default
  int beats_per_bar = 4;
  bool snap_to_grid = true;
  bool percussive_notes = false;

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
};
typedef int PianoRollControl;

static auto constexpr PianoRollControl_DefaultFlags =
    PianoRollControl_ShowPercussiveSwitch | PianoRollControl_ShowSnapToGridSwitch |
    PianoRollControl_ShowBeatsPerBarSetting | PianoRollControl_ShowSubBeatGridSetting;

namespace ImGui {
void FitPianoRollVertical(PianoRollState& state, int minPitch, int maxPitch, const ImVec2& canvasSize,
                         float contentPadding = 48.0f);
void FitPianoRollHorizontal(PianoRollState& state, double totalBeats, const ImVec2& canvasSize,
                           float contentPadding = 48.0f);
void FitPianoRollFullLength(PianoRollState& state, double fullLengthBeats, const ImVec2& canvasSize,
                           float contentPadding = 48.0f);
void FitPianoRollToContent(PianoRollState& state, int minPitch, int maxPitch, double totalBeats,
                           const ImVec2& canvasSize, float contentPadding = 48.0f);

// Startet den Canvas
bool BeginPianoRoll(const char* str_id, PianoRollState& state, const ImVec2& size = ImVec2(0, 0),
                    PianoRollControl flags = PianoRollControl_DefaultFlags);

// Erstellt eine neue Note per Single-Click / Touch (inklusive Drag zum Aufziehen)
bool PianoRollCreateNote(PianoRollState& state, double* outBeat, int* outPitch, int* outVelocity, double* outDuration);

inline ImU32 constexpr PianoRollDefaultNoteColor(bool active, bool hovered_default, bool hovered_resize_zone,
                                                 int velocity) {
  if (active || hovered_resize_zone) return IM_COL32(127 + velocity, 255 - velocity, 127, 255);
  if (hovered_default) return IM_COL32(107 + velocity, 235 - velocity, 107, 255);
  return IM_COL32(87 + velocity, 215 - velocity, 87, 255);
}

// Rendert eine Note und verarbeitet Interaktionen direkt über Zeiger
PianoRollInteraction PianoRollNote(PianoRollState& state, uint32_t id, double* beat, int* pitch, int* velocity,
                                   double* duration, std::function<ImU32(bool, bool, bool, int)> f_color = nullptr);

// Schließt den Canvas und zeichnet Overlays
void EndPianoRoll(PianoRollState& state);

// Demo
void ShowPianoRollDemoApp();

}  // namespace ImGui
