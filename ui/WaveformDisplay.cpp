#include "WaveformDisplay.h"

// =========================================================================
// Construction
// =========================================================================

WaveformDisplay::WaveformDisplay()
{
    setOpaque(true);
}

// =========================================================================
// Data
// =========================================================================

void WaveformDisplay::setWaveformData(const std::array<float, 128>& data)
{
    peaks = data;
    hasData = true;
    repaint();
}

void WaveformDisplay::setWaveformData(const float* data, int numPoints)
{
    peaks.fill(0.0f);
    if (data != nullptr && numPoints > 0)
    {
        hasData = true;
        const float ratio = static_cast<float>(numPoints) / 128.0f;
        for (int i = 0; i < 128; ++i)
        {
            int srcIdx = juce::jmin(static_cast<int>(i * ratio), numPoints - 1);
            peaks[static_cast<size_t>(i)] = data[srcIdx];
        }
    }
    else
    {
        hasData = false;
    }
    repaint();
}

void WaveformDisplay::clearWaveform()
{
    peaks.fill(0.0f);
    hasData = false;
    repaint();
}

// =========================================================================
// Markers
// =========================================================================

void WaveformDisplay::setMarkersEnabled(bool enabled)
{
    markersEnabled = enabled;
    repaint();
}

void WaveformDisplay::setMarkerPositions(float startNorm, float endNorm)
{
    startMarkerNorm = juce::jlimit(0.0f, 1.0f, startNorm);
    endMarkerNorm   = juce::jlimit(0.0f, 1.0f, endNorm);
    repaint();
}

// =========================================================================
// Coordinate helpers
// =========================================================================

float WaveformDisplay::xToNorm(float x) const
{
    return juce::jlimit(0.0f, 1.0f, x / static_cast<float>(getWidth()));
}

float WaveformDisplay::normToX(float norm) const
{
    return norm * static_cast<float>(getWidth());
}

// =========================================================================
// Paint
// =========================================================================

void WaveformDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.fillAll(VoidLookAndFeel::voidBlack());

    // Grid lines
    if (drawGrid)
    {
        g.setColour(VoidLookAndFeel::ghost());
        // Horizontal centre line
        float cy = bounds.getCentreY();
        g.drawHorizontalLine(static_cast<int>(cy), bounds.getX(), bounds.getRight());

        // Vertical quarter lines
        for (int i = 1; i < 4; ++i)
        {
            float x = bounds.getX() + bounds.getWidth() * (i / 4.0f);
            g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
        }
    }

    if (!hasData)
        return;

    // Draw waveform
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();
    const float cy = bounds.getCentreY();
    const float halfH = h * 0.45f; // Leave a little padding

    if (filled)
    {
        juce::Path path;
        path.startNewSubPath(bounds.getX(), cy);

        // Top half (positive peaks)
        for (int i = 0; i < 128; ++i)
        {
            float x = bounds.getX() + (static_cast<float>(i) / 127.0f) * w;
            float y = cy - peaks[static_cast<size_t>(i)] * halfH;
            path.lineTo(x, y);
        }

        // Bottom half (mirror) -- walk back
        for (int i = 127; i >= 0; --i)
        {
            float x = bounds.getX() + (static_cast<float>(i) / 127.0f) * w;
            float y = cy + peaks[static_cast<size_t>(i)] * halfH;
            path.lineTo(x, y);
        }

        path.closeSubPath();

        // Filled waveform with translucent cyan
        g.setColour(VoidLookAndFeel::accentCyan().withAlpha(0.25f));
        g.fillPath(path);

        // Outline
        g.setColour(VoidLookAndFeel::accentCyan());
        g.strokePath(path, juce::PathStrokeType(1.0f));
    }
    else
    {
        // Line-only mode (for thumbnails)
        juce::Path path;
        for (int i = 0; i < 128; ++i)
        {
            float x = bounds.getX() + (static_cast<float>(i) / 127.0f) * w;
            float y = cy - peaks[static_cast<size_t>(i)] * halfH;
            if (i == 0)
                path.startNewSubPath(x, y);
            else
                path.lineTo(x, y);
        }
        g.setColour(VoidLookAndFeel::accentCyan());
        g.strokePath(path, juce::PathStrokeType(1.0f));
    }

    // Markers
    if (markersEnabled)
    {
        auto drawMarker = [&](float normPos)
        {
            float x = normToX(normPos);
            g.setColour(VoidLookAndFeel::accentMagenta());
            g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
            // Small triangle at top
            juce::Path tri;
            tri.addTriangle(x - 4.0f, bounds.getY(),
                            x + 4.0f, bounds.getY(),
                            x, bounds.getY() + 6.0f);
            g.fillPath(tri);
        };

        drawMarker(startMarkerNorm);
        drawMarker(endMarkerNorm);

        // Dim the regions outside the markers
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        float sx = normToX(startMarkerNorm);
        float ex = normToX(endMarkerNorm);
        if (sx > bounds.getX())
            g.fillRect(bounds.getX(), bounds.getY(), sx - bounds.getX(), h);
        if (ex < bounds.getRight())
            g.fillRect(ex, bounds.getY(), bounds.getRight() - ex, h);
    }
}

void WaveformDisplay::resized() {}

// =========================================================================
// Mouse interaction (marker dragging)
// =========================================================================

void WaveformDisplay::mouseDown(const juce::MouseEvent& e)
{
    if (!markersEnabled) return;

    float mx = static_cast<float>(e.x);
    float sx = normToX(startMarkerNorm);
    float ex = normToX(endMarkerNorm);

    if (std::abs(mx - sx) < kMarkerHitZonePx)
        currentDrag = DragTarget::Start;
    else if (std::abs(mx - ex) < kMarkerHitZonePx)
        currentDrag = DragTarget::End;
    else
        currentDrag = DragTarget::None;
}

void WaveformDisplay::mouseDrag(const juce::MouseEvent& e)
{
    if (currentDrag == DragTarget::None) return;

    float norm = xToNorm(static_cast<float>(e.x));

    if (currentDrag == DragTarget::Start)
    {
        startMarkerNorm = juce::jlimit(0.0f, endMarkerNorm - 0.01f, norm);
    }
    else if (currentDrag == DragTarget::End)
    {
        endMarkerNorm = juce::jlimit(startMarkerNorm + 0.01f, 1.0f, norm);
    }

    repaint();

    if (onMarkersChanged)
        onMarkersChanged(startMarkerNorm, endMarkerNorm);
}

void WaveformDisplay::mouseUp(const juce::MouseEvent&)
{
    currentDrag = DragTarget::None;
}
