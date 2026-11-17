#include <algorithm>
#include <cstddef>
#include <vector>

#include "imgui-pianoroll/imgui-pianoroll.h"

struct CustomNote {
  uint32_t id;
  int pitch;
  int velocity;
  double beat;
  double duration;
  bool isMuted = false;
};

void ImGui::ShowPianoRollDemoApp() {
  static PianoRollState synthState = PianoRollState::CreateDefaultMelodic();
  static PianoRollState drumState = PianoRollState::CreateDefaultPercussive();

  synthState.fitMinPitch = 48;
  synthState.fitMaxPitch = 84;
  drumState.fitMinPitch = 35;
  drumState.fitMaxPitch = 45;

  static std::vector<CustomNote> synthNotes = {
      {1, 60, 100, 0.0, 1.0},
      {2, 64, 100, 1.0, 1.0},
      {3, 67, 100, 2.0, 2.0}
  };

  static std::vector<CustomNote> drumNotes = {
      {100, 36, 100, 0.0, 0.0},
      {101, 38, 100, 1.0, 0.0},
      {102, 36, 100, 2.0, 0.0},
      {103, 38, 100, 3.0, 0.0}
  };

  static uint32_t nextId = 1000;

  synthState.velocity_commit = [](uint32_t id, int velocity) {
    for (auto& note : synthNotes) if (note.id == id) note.velocity = velocity;
  };
  drumState.velocity_commit = [](uint32_t id, int velocity) {
    for (auto& note : drumNotes) if (note.id == id) note.velocity = velocity;
  };
  synthState.delete_note = [](uint32_t id) {
    synthNotes.erase(std::remove_if(synthNotes.begin(), synthNotes.end(), [&](auto const& n) { return n.id == id; }),
                     synthNotes.end());
  };
  drumState.delete_note = [](uint32_t id) {
    drumNotes.erase(std::remove_if(drumNotes.begin(), drumNotes.end(), [&](auto const& n) { return n.id == id; }),
                    drumNotes.end());
  };
  ImGui::UpdatePianoRollVelocity(synthState);
  ImGui::UpdatePianoRollVelocity(drumState);

  ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("MIDI Editor Demo")) {
    bool const fitSynth = ImGui::Button("Fit Synth to Content");
    ImGui::SameLine();
    bool const fitDrums = ImGui::Button("Fit Drums to Content");
    ImGui::Separator();

    ImGui::Text("Synthesizer Track (Links-Klick = Erstellen/Ziehen, Rechts-Klick = Mute/Unmute)");
    // Reserve the separator, drum label, and spacing before splitting the remaining height.
    float const overhead = ImGui::GetTextLineHeight() + 3.0f * ImGui::GetStyle().ItemSpacing.y;
    float const editorHeight = std::max(1.0f, (ImGui::GetContentRegionAvail().y - overhead) * 0.5f);
    if (ImGui::BeginPianoRoll("SynthEditor", synthState, ImVec2(0, editorHeight),
                               PianoRollControl_DefaultFlags | PianoRollControl_ShowVelocityEditorSwitch)) {
      if (fitSynth) {
        ImGui::FitPianoRollToContent(synthState, 48, 84, 16.0, ImGui::GetContentRegionAvail());
      }
      double newBeat = 0.0, newDuration = 0.0;
      int newPitch = 0, newVelocity = 0;
      if (ImGui::PianoRollCreateNote(synthState, &newBeat, &newPitch, &newVelocity, &newDuration)) {
        synthNotes.push_back({nextId++, newPitch, newVelocity, newBeat, newDuration});
      }

      for (auto it = synthNotes.begin(); it != synthNotes.end();) {
        PianoRollInteraction flags = ImGui::PianoRollNote(synthState, it->id, &it->beat, &it->pitch, &it->velocity,
                                                          &it->duration, [&](bool active, bool hovered, bool resize, int vel) {
                                                            if (it->isMuted) return IM_COL32(100, 100, 100, 255);
                                                            return PianoRollDefaultNoteColor(active, hovered, resize, vel);
                                                          });

        if (flags & PianoRollInteraction_RightClicked) {
          it->isMuted = !it->isMuted;
        }

        ++it;
      }

      ImGui::EndPianoRoll(synthState);
    }

    ImGui::Separator();

    ImGui::Text("Drum Track (Percussive Diamonds Mode)");
    if (ImGui::BeginPianoRoll("DrumEditor", drumState, ImVec2(0, std::max(1.0f, ImGui::GetContentRegionAvail().y)),
                               PianoRollControl_DefaultFlags | PianoRollControl_ShowVelocityEditorSwitch)) {
      if (fitDrums) {
        ImGui::FitPianoRollToContent(drumState, 35, 45, 8.0, ImGui::GetContentRegionAvail());
      }
      double newBeat = 0.0, newDuration = 0.0;
      int newPitch = 0, newVelocity = 0;
      if (ImGui::PianoRollCreateNote(drumState, &newBeat, &newPitch, &newVelocity, &newDuration)) {
        drumNotes.push_back({nextId++, newPitch, newVelocity, newBeat, 0.0});
      }

      for (auto& note : drumNotes) {
        ImGui::PianoRollNote(drumState, note.id, &note.beat, &note.pitch, &note.velocity, &note.duration);
      }

      ImGui::EndPianoRoll(drumState);
    }
  }
  ImGui::End();
}
