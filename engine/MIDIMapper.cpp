#include "MIDIMapper.h"
#include <cmath>

namespace void_drum {

// =========================================================================
// Construction
// =========================================================================

MIDIMapper::MIDIMapper()
{
    // Initialise per-pad velocity curves to "use global" (-1)
    for (auto& v : padVelocityCurves)
        v.store(-1, std::memory_order_relaxed);

    resetToDefaults();
}

// =========================================================================
// IMIDIMapper interface
// =========================================================================

MIDIMapping MIDIMapper::getMapping() const
{
    MIDIMapping m;
    {
        std::lock_guard<std::mutex> lock(mappingMutex);
        for (const auto& [note, pad] : noteToPad)
            m.noteToPad[note] = pad;
    }
    m.velocityCurve = globalVelocityCurve.load(std::memory_order_relaxed);
    return m;
}

void MIDIMapper::setMapping(const MIDIMapping& mapping)
{
    {
        std::lock_guard<std::mutex> lock(mappingMutex);
        noteToPad.clear();
        padToNote.clear();
        for (const auto& [note, pad] : mapping.noteToPad)
        {
            noteToPad[note] = pad;
            padToNote[pad] = note;
        }
    }
    globalVelocityCurve.store(mapping.velocityCurve, std::memory_order_relaxed);
}

void MIDIMapper::midiLearn(int padIndex)
{
    learningPad.store(padIndex, std::memory_order_release);
}

// =========================================================================
// Note routing
// =========================================================================

int MIDIMapper::getPadForNote(int midiNote) const
{
    std::lock_guard<std::mutex> lock(mappingMutex);
    auto it = noteToPad.find(midiNote);
    return (it != noteToPad.end()) ? it->second : -1;
}

int MIDIMapper::getNoteForPad(int padIndex) const
{
    std::lock_guard<std::mutex> lock(mappingMutex);
    auto it = padToNote.find(padIndex);
    return (it != padToNote.end()) ? it->second : -1;
}

void MIDIMapper::assignNoteToPad(int midiNote, int padIndex)
{
    std::lock_guard<std::mutex> lock(mappingMutex);

    // Remove any previous mapping for this note
    auto noteIt = noteToPad.find(midiNote);
    if (noteIt != noteToPad.end())
    {
        int oldPad = noteIt->second;
        padToNote.erase(oldPad);
        noteToPad.erase(noteIt);
    }

    // Remove any previous note for this pad
    auto padIt = padToNote.find(padIndex);
    if (padIt != padToNote.end())
    {
        int oldNote = padIt->second;
        noteToPad.erase(oldNote);
        padToNote.erase(padIt);
    }

    // Create the new mapping
    noteToPad[midiNote] = padIndex;
    padToNote[padIndex] = midiNote;
}

void MIDIMapper::resetToDefaults()
{
    auto defaultMap = createDefaultMIDIMapping();
    setMapping(defaultMap);
}

// =========================================================================
// Velocity curves
// =========================================================================

float MIDIMapper::computeVelocityGain(int velocity, VelocityCurve curve)
{
    float norm = static_cast<float>(juce::jlimit(0, 127, velocity)) / 127.0f;

    switch (curve)
    {
        case VelocityCurve::Exponential:
            return norm * norm; // pow(norm, 2.0)

        case VelocityCurve::SCurve:
        {
            // Smoothstep: 3x^2 - 2x^3
            float t = juce::jlimit(0.0f, 1.0f, norm);
            return t * t * (3.0f - 2.0f * t);
        }

        case VelocityCurve::Linear:
        default:
            return norm;
    }
}

float MIDIMapper::applyVelocityCurve(int velocity) const
{
    return computeVelocityGain(velocity,
                               globalVelocityCurve.load(std::memory_order_relaxed));
}

float MIDIMapper::applyVelocityCurve(int velocity, int padIndex) const
{
    if (padIndex >= 0 && padIndex < NUM_PADS)
    {
        int padCurve = padVelocityCurves[static_cast<size_t>(padIndex)]
                           .load(std::memory_order_relaxed);
        if (padCurve >= 0)
            return computeVelocityGain(velocity, static_cast<VelocityCurve>(padCurve));
    }
    return applyVelocityCurve(velocity);
}

void MIDIMapper::setGlobalVelocityCurve(VelocityCurve curve)
{
    globalVelocityCurve.store(curve, std::memory_order_relaxed);
}

VelocityCurve MIDIMapper::getGlobalVelocityCurve() const
{
    return globalVelocityCurve.load(std::memory_order_relaxed);
}

void MIDIMapper::setPadVelocityCurve(int padIndex, VelocityCurve curve)
{
    if (padIndex >= 0 && padIndex < NUM_PADS)
        padVelocityCurves[static_cast<size_t>(padIndex)]
            .store(static_cast<int>(curve), std::memory_order_relaxed);
}

bool MIDIMapper::hasPadVelocityCurve(int padIndex) const
{
    if (padIndex >= 0 && padIndex < NUM_PADS)
        return padVelocityCurves[static_cast<size_t>(padIndex)]
                   .load(std::memory_order_relaxed) >= 0;
    return false;
}

// =========================================================================
// MIDI Learn state
// =========================================================================

bool MIDIMapper::isLearning() const
{
    return learningPad.load(std::memory_order_acquire) >= 0;
}

int MIDIMapper::getLearningPad() const
{
    return learningPad.load(std::memory_order_acquire);
}

// =========================================================================
// MIDI CC mapping
// =========================================================================

void MIDIMapper::mapCCToParameter(int ccNumber, const juce::String& parameterId)
{
    std::lock_guard<std::mutex> lock(ccMutex);
    ccToParameter[ccNumber] = parameterId;
}

void MIDIMapper::unmapCC(int ccNumber)
{
    std::lock_guard<std::mutex> lock(ccMutex);
    ccToParameter.erase(ccNumber);
}

juce::String MIDIMapper::getParameterForCC(int ccNumber) const
{
    std::lock_guard<std::mutex> lock(ccMutex);
    auto it = ccToParameter.find(ccNumber);
    return (it != ccToParameter.end()) ? it->second : juce::String();
}

void MIDIMapper::processCCMessage(int ccNumber, int ccValue,
                                  juce::AudioProcessorValueTreeState& apvts) const
{
    juce::String paramId;
    {
        std::lock_guard<std::mutex> lock(ccMutex);
        auto it = ccToParameter.find(ccNumber);
        if (it == ccToParameter.end())
            return;
        paramId = it->second;
    }

    if (auto* param = apvts.getRawParameterValue(paramId))
    {
        // Get the parameter's range to map the CC 0-127 value
        if (auto* rangedParam = apvts.getParameter(paramId))
        {
            float normalisedValue = static_cast<float>(ccValue) / 127.0f;
            rangedParam->setValueNotifyingHost(normalisedValue);
        }
    }
}

// =========================================================================
// MIDI processing (called from processBlock)
// =========================================================================

int MIDIMapper::processNoteOn(int midiNote, int /*velocity*/)
{
    // Check learn mode first (atomic read)
    int targetPad = learningPad.load(std::memory_order_acquire);
    if (targetPad >= 0 && targetPad < NUM_PADS)
    {
        assignNoteToPad(midiNote, targetPad);
        learningPad.store(-1, std::memory_order_release); // exit learn mode
        return targetPad;
    }

    // Normal routing
    return getPadForNote(midiNote);
}

// =========================================================================
// Serialisation
// =========================================================================

juce::ValueTree MIDIMapper::toValueTree() const
{
    juce::ValueTree tree("MIDIMapper");

    // Note-to-pad mapping
    juce::ValueTree noteMap("NoteMap");
    {
        std::lock_guard<std::mutex> lock(mappingMutex);
        for (const auto& [note, pad] : noteToPad)
        {
            juce::ValueTree entry("Entry");
            entry.setProperty("note", note, nullptr);
            entry.setProperty("pad", pad, nullptr);
            noteMap.addChild(entry, -1, nullptr);
        }
    }
    tree.addChild(noteMap, -1, nullptr);

    // Global velocity curve
    tree.setProperty("velocityCurve",
                     static_cast<int>(globalVelocityCurve.load(std::memory_order_relaxed)),
                     nullptr);

    // Per-pad velocity curves
    juce::ValueTree padCurves("PadVelocityCurves");
    for (int i = 0; i < NUM_PADS; ++i)
    {
        int cv = padVelocityCurves[static_cast<size_t>(i)]
                     .load(std::memory_order_relaxed);
        if (cv >= 0)
        {
            juce::ValueTree entry("Pad");
            entry.setProperty("index", i, nullptr);
            entry.setProperty("curve", cv, nullptr);
            padCurves.addChild(entry, -1, nullptr);
        }
    }
    tree.addChild(padCurves, -1, nullptr);

    // CC mappings
    juce::ValueTree ccMap("CCMap");
    {
        std::lock_guard<std::mutex> lock(ccMutex);
        for (const auto& [cc, paramId] : ccToParameter)
        {
            juce::ValueTree entry("CC");
            entry.setProperty("cc", cc, nullptr);
            entry.setProperty("param", paramId, nullptr);
            ccMap.addChild(entry, -1, nullptr);
        }
    }
    tree.addChild(ccMap, -1, nullptr);

    return tree;
}

void MIDIMapper::fromValueTree(const juce::ValueTree& tree)
{
    if (!tree.isValid() || tree.getType().toString() != "MIDIMapper")
        return;

    // Note map
    auto noteMap = tree.getChildWithName("NoteMap");
    if (noteMap.isValid())
    {
        std::lock_guard<std::mutex> lock(mappingMutex);
        noteToPad.clear();
        padToNote.clear();
        for (int i = 0; i < noteMap.getNumChildren(); ++i)
        {
            auto entry = noteMap.getChild(i);
            int note = entry.getProperty("note", -1);
            int pad  = entry.getProperty("pad", -1);
            if (note >= 0 && note <= 127 && pad >= 0 && pad < NUM_PADS)
            {
                noteToPad[note] = pad;
                padToNote[pad] = note;
            }
        }
    }

    // Global velocity curve
    int curveInt = tree.getProperty("velocityCurve", 0);
    if (curveInt >= 0 && curveInt <= 2)
        globalVelocityCurve.store(static_cast<VelocityCurve>(curveInt),
                                  std::memory_order_relaxed);

    // Per-pad velocity curves
    auto padCurves = tree.getChildWithName("PadVelocityCurves");
    for (auto& v : padVelocityCurves)
        v.store(-1, std::memory_order_relaxed);
    if (padCurves.isValid())
    {
        for (int i = 0; i < padCurves.getNumChildren(); ++i)
        {
            auto entry = padCurves.getChild(i);
            int idx = entry.getProperty("index", -1);
            int cv  = entry.getProperty("curve", -1);
            if (idx >= 0 && idx < NUM_PADS && cv >= 0)
                padVelocityCurves[static_cast<size_t>(idx)]
                    .store(cv, std::memory_order_relaxed);
        }
    }

    // CC mappings
    auto ccMap = tree.getChildWithName("CCMap");
    {
        std::lock_guard<std::mutex> lock(ccMutex);
        ccToParameter.clear();
        if (ccMap.isValid())
        {
            for (int i = 0; i < ccMap.getNumChildren(); ++i)
            {
                auto entry = ccMap.getChild(i);
                int cc = entry.getProperty("cc", -1);
                juce::String paramId = entry.getProperty("param", "").toString();
                if (cc >= 0 && cc <= 127 && paramId.isNotEmpty())
                    ccToParameter[cc] = paramId;
            }
        }
    }
}

} // namespace void_drum
