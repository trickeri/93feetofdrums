#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "VoidLookAndFeel.h"
#include <array>
#include <functional>

// =========================================================================
// WaveformDisplay
//
// Reusable waveform visualiser drawn from a 128-point normalised peak array.
// Used in the FXPanel (large, with draggable start/end markers) and inline
// in the SampleBrowser (tiny thumbnail mode).
//
// Colours:
//   Background  -- void-black (#000000)
//   Waveform    -- accent-cyan (#00F0FF)
//   Grid lines  -- ghost (#333333)
//   Markers     -- accent-magenta (#FF00AA)
// =========================================================================
class WaveformDisplay : public juce::Component
{
public:
    // Callback fired when the user drags a start or end marker.
    // Arguments: normalised position (0..1).
    std::function<void(float startNorm, float endNorm)> onMarkersChanged;

    WaveformDisplay();
    ~WaveformDisplay() override = default;

    // -- Data ----------------------------------------------------------------

    /** Set the waveform peak data to display. */
    void setWaveformData(const std::array<float, 128>& peaks);

    /** Set the waveform data from a raw float pointer (for inline thumbnail use). */
    void setWaveformData(const float* data, int numPoints);

    /** Clear waveform data (draws empty). */
    void clearWaveform();

    // -- Markers -------------------------------------------------------------

    /** Enable or disable start/end markers (draggable vertical lines). */
    void setMarkersEnabled(bool enabled);

    /** Set marker positions in normalised 0..1 range. */
    void setMarkerPositions(float startNorm, float endNorm);

    float getStartMarker() const { return startMarkerNorm; }
    float getEndMarker()   const { return endMarkerNorm; }

    // -- Drawing options -----------------------------------------------------

    /** If true, draws as a filled shape rather than a line (default: true). */
    void setFilled(bool shouldFill) { filled = shouldFill; repaint(); }

    /** If true, draws background grid lines (default: true for large, false for thumbnail). */
    void setDrawGrid(bool shouldDrawGrid) { drawGrid = shouldDrawGrid; repaint(); }

    // -- Component overrides -------------------------------------------------
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    std::array<float, 128> peaks {};
    bool hasData = false;

    bool markersEnabled = false;
    float startMarkerNorm = 0.0f;
    float endMarkerNorm   = 1.0f;

    bool filled   = true;
    bool drawGrid = true;

    // Marker dragging state
    enum class DragTarget { None, Start, End };
    DragTarget currentDrag = DragTarget::None;

    static constexpr int kMarkerHitZonePx = 6;

    float xToNorm(float x) const;
    float normToX(float norm) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};
