// SPDX-FileCopyrightText: 2019-2024 Connor McLaughlin <stenzek@gmail.com>
// SPDX-FileCopyrightText: 2026 mentalfoundry
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include "types.h"

#include <array>
#include <memory>

class StateWrapper;

class AudioStream;
struct AudioStreamParameters;

namespace SPU {

enum : u32
{
  RAM_SIZE = 512 * 1024,
  RAM_MASK = RAM_SIZE - 1,
  SAMPLE_RATE = 44100,
};

void Initialize();
void CPUClockChanged();
void Shutdown();
void Reset();
bool DoState(StateWrapper& sw);

u16 ReadRegister(u32 offset);
void WriteRegister(u32 offset, u16 value);

void DMARead(u32* words, u32 word_count);
void DMAWrite(const u32* words, u32 word_count);

// Render statistics debug window.
void DrawDebugStateWindow();

// Executes the SPU, generating any pending samples.
void GeneratePendingSamples();

/// Access to SPU RAM.
const std::array<u8, RAM_SIZE>& GetRAM();
std::array<u8, RAM_SIZE>& GetWritableRAM();

/// Change output stream - used for runahead.
// TODO: Make it use system "running ahead" flag
bool IsAudioOutputMuted();
void SetAudioOutputMuted(bool muted);

AudioStream* GetOutputStream();

/// Lets a frontend supply the output stream instead of the configured audio backend, for frontends that collect the
/// samples themselves, such as libretro. Takes effect the next time the stream is created. nullptr restores the
/// configured backend.
using OutputStreamFactory = std::unique_ptr<AudioStream> (*)(u32 sample_rate, const AudioStreamParameters& parameters);
void SetOutputStreamFactory(OutputStreamFactory factory);
void RecreateOutputStream();

}; // namespace SPU
