// SPDX-FileCopyrightText: 2026 mentalfoundry
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include "memory_card_image.h"
#include "timing_event.h"
#include "types.h"

#include <functional>
#include <memory>
#include <span>
#include <vector>

class Error;

struct psemu;

// PocketStation in a memory card slot. The device's BIOS implements the memory card protocol, so this only passes
// bytes to the emulated device, and keeps it in step with console time.
//
// The device is not part of save states, only its flash is, as the card data. Loading a state does not rewind it.
class PocketStation
{
public:
  ~PocketStation();

  /// Boots and docks a device with the given BIOS and card image. slot selects the device's serial number.
  /// flash_written is called when the device has written to its flash.
  static std::unique_ptr<PocketStation> Create(std::span<const u8> bios, const MemoryCardImage::DataArray& flash,
                                               u32 slot, std::function<void()> flash_written, Error* error);

  /// The console asserted /SEL. The device reacts to it before the first byte arrives.
  void Select();

  /// Exchanges one byte. Returns true if the device acknowledges, GetACKTicks() from now.
  bool Transfer(u8 data_in, u8* data_out);
  TickCount GetACKTicks() const { return m_ack_ticks; }

  /// Releases /SEL at the end of a transaction. was_selected is false if the device took no part in it.
  void ResetTransferState(bool was_selected);

  /// Copies the device's flash to data. Returns true if it differed.
  bool SaveFlash(MemoryCardImage::DataArray* data);

  /// Called before the final save of the card. If an app is running, has it save its progress to flash.
  /// The device does not run afterwards.
  void PrepareForSave();

private:
  explicit PocketStation(u32 slot);

  static void EventCallback(void* param, TickCount ticks, TickCount ticks_late);

  /// Runs the device up to the current console time.
  void Sync(bool periodic);

  /// Brings the device to exactly the current console time, before an input from the console.
  void CatchUp();

  void RunTo(GlobalTicks now);
  void Rebase(GlobalTicks now);
  bool CheckFlashWritten();

  TimingEvent m_event;

  psemu* m_ps = nullptr;
  u32 m_slot = 0;

  // Device time run since console time m_anchor_ticks, in PSEMU_TIME_HZ units.
  GlobalTicks m_anchor_ticks = 0;
  u64 m_units_run = 0;

  // Device state when the last byte arrived, so that running to its acknowledge can be undone if the console's next
  // input comes first. Not needed once the console passes m_ahead_until.
  std::vector<u8> m_saved_state;
  u64 m_saved_units = 0;
  u64 m_ahead_until = 0;
  bool m_saved_valid = false;

  bool m_finished = false;

  TickCount m_ack_ticks = 0;

  std::vector<u8> m_flash_snapshot;
  std::function<void()> m_flash_written;
};