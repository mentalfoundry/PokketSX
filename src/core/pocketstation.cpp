// SPDX-FileCopyrightText: 2026 mentalfoundry
// SPDX-License-Identifier: GPL-3.0

#include "pocketstation.h"
#include "cpu_core.h"
#include "system.h"

#include "common/error.h"
#include "common/log.h"

#include "psemu/psemu.h"

#include <algorithm>
#include <cstring>
#include <numeric>

Log_SetChannel(PocketStation);

namespace {

// One frame of the device, which refreshes at approximately 32Hz, in reference cycles.
constexpr u32 FRAME_CYCLES = PSEMU_REFERENCE_CLOCK_HZ / 32;

// Frames to run before docking, so the BIOS reaches its shell.
constexpr u32 BOOT_FRAMES = 200;

// Frames to wait for the kernel to enable communication after docking. It debounces the dock signal.
constexpr u32 DOCK_FRAMES = 60;

// Device time is kept in PSEMU_TIME_HZ units. CONSOLE_TICKS_PER_STEP console ticks are exactly TIME_UNITS_PER_STEP
// units, so conversion is exact, and the two clocks never drift apart.
constexpr u64 CLOCK_GCD = std::gcd(static_cast<u64>(PSEMU_TIME_HZ), static_cast<u64>(System::MASTER_CLOCK));
constexpr u64 TIME_UNITS_PER_STEP = PSEMU_TIME_HZ / CLOCK_GCD;
constexpr u64 CONSOLE_TICKS_PER_STEP = System::MASTER_CLOCK / CLOCK_GCD;
static_assert(TIME_UNITS_PER_STEP == 13312640 && CONSOLE_TICKS_PER_STEP == 441);

// Console ticks to device units, rounded down. Split so the product cannot overflow.
constexpr u64 TicksToUnits(u64 ticks)
{
  return (ticks / CONSOLE_TICKS_PER_STEP) * TIME_UNITS_PER_STEP +
         (ticks % CONSOLE_TICKS_PER_STEP) * TIME_UNITS_PER_STEP / CONSOLE_TICKS_PER_STEP;
}

// Device units to console ticks, rounded up.
constexpr u64 UnitsToTicksCeil(u64 units)
{
  return (units / TIME_UNITS_PER_STEP) * CONSOLE_TICKS_PER_STEP +
         ((units % TIME_UNITS_PER_STEP) * CONSOLE_TICKS_PER_STEP + TIME_UNITS_PER_STEP - 1u) / TIME_UNITS_PER_STEP;
}

// The device runs at least every 1/128s, even when nothing accesses it.
constexpr TickCount SYNC_PERIOD_TICKS = static_cast<TickCount>(System::MASTER_CLOCK / 128u);

// A larger debt can only come from the console clock jumping forward.
constexpr u64 MAX_CATCH_UP_UNITS = PSEMU_TIME_HZ * 4u;

// How far to run looking for the acknowledge of a byte. The last byte of a command is never acknowledged, so it runs
// the whole budget, which is then undone when /SEL is released.
constexpr u64 ACK_BUDGET_UNITS = static_cast<u64>(PSEMU_COM_DEFAULT_TIMEOUT_CYCLES) * PSEMU_TIME_PER_REFERENCE_CYCLE;

// A larger lead can only come from the console clock going backwards.
constexpr u64 MAX_LEAD_UNITS = ACK_BUDGET_UNITS * 2u;

// Frame limits for PrepareForSave(). The kernel programs flash after /SEL is released, so a Write Sector sent just
// before shutdown needs time to complete.
constexpr u32 WRITE_SECTOR_SETTLE_FRAMES = 50;
constexpr u32 UNDOCK_FRAMES = 600;
constexpr u32 SAVE_BUTTON_HOLD_FRAMES = 300;

constexpr const char* s_event_names[NUM_CONTROLLER_AND_CARD_PORTS] = {
  "PocketStation 1", "PocketStation 2", "PocketStation 3", "PocketStation 4",
  "PocketStation 5", "PocketStation 6", "PocketStation 7", "PocketStation 8",
};

// Includes ticks the CPU has run, but not yet added to the timing system.
GlobalTicks GetConsoleNow()
{
  return TimingEvents::GetGlobalTickCounter() + CPU::GetPendingTicks();
}

} // namespace

PocketStation::PocketStation(u32 slot)
  : m_event(s_event_names[slot % NUM_CONTROLLER_AND_CARD_PORTS], SYNC_PERIOD_TICKS, SYNC_PERIOD_TICKS,
            &PocketStation::EventCallback, this),
    m_slot(slot)
{
}

PocketStation::~PocketStation()
{
  m_event.Deactivate();

  if (m_ps)
    psemu_destroy(m_ps);
}

std::unique_ptr<PocketStation> PocketStation::Create(std::span<const u8> bios, const MemoryCardImage::DataArray& flash,
                                                     u32 slot, std::function<void()> flash_written, Error* error)
{
  std::unique_ptr<PocketStation> ret(new PocketStation(slot));
  ret->m_flash_written = std::move(flash_written);

  ret->m_ps = psemu_create();
  if (!ret->m_ps)
  {
    Error::SetStringView(error, "Failed to allocate PocketStation machine.");
    return nullptr;
  }

  if (psemu_load_bios(ret->m_ps, bios.data(), bios.size()) != PSEMU_OK)
  {
    Error::SetStringFmt(error, "PocketStation BIOS must be {} bytes, got {}.", static_cast<u32>(PSEMU_BIOS_SIZE),
                        bios.size());
    return nullptr;
  }

  if (psemu_load_flash_image(ret->m_ps, flash.data(), flash.size()) != PSEMU_OK)
  {
    Error::SetStringView(error, "Failed to load flash image into PocketStation.");
    return nullptr;
  }

  ret->m_saved_state.resize(psemu_snapshot_size());

  psemu_reset(ret->m_ps);

  // Must be set before the device runs, since an app reads it when it creates a save.
  psemu_set_hardware_id(ret->m_ps, slot + 1);

  for (u32 i = 0; i < BOOT_FRAMES; i++)
    psemu_run(ret->m_ps, FRAME_CYCLES);

  psemu_com_set_docked(ret->m_ps, 1);

  bool enabled = false;
  for (u32 i = 0; i < DOCK_FRAMES; i++)
  {
    psemu_run(ret->m_ps, FRAME_CYCLES);
    if (psemu_com_is_enabled(ret->m_ps))
    {
      enabled = true;
      break;
    }
  }

  if (!enabled)
  {
    Error::SetStringView(error, "PocketStation BIOS did not enable communication after docking.");
    return nullptr;
  }

  // Boot and docking happen before console time starts for the device.
  ret->Rebase(GetConsoleNow());
  if (const u8* const fl = psemu_flash_data(ret->m_ps))
    ret->m_flash_snapshot.assign(fl, fl + PSEMU_FLASH_SIZE);
  ret->m_event.Activate();

  return ret;
}

void PocketStation::PrepareForSave()
{
  if (!m_ps || m_finished)
    return;

  // Everything below happens after console time ends for the device, so nothing may undo it.
  CatchUp();
  m_event.Deactivate();
  m_saved_valid = false;
  m_finished = true;

  for (u32 i = 0; i < WRITE_SECTOR_SETTLE_FRAMES; i++)
    psemu_run(m_ps, FRAME_CYCLES);

  // An app keeps its progress in RAM, and writes it to flash when the player leaves it. A real device keeps its RAM on
  // battery after it is unplugged, but this one ends with the session. So do what the player would: undock it, and
  // press the button that leaves the app. If no app is running, the game did not start one, and there is nothing to do.
  const u8* const fl = psemu_flash_data(m_ps);
  if (!fl || !psemu_app_running(m_ps))
    return;

  const std::vector<u8> flash_before(fl, fl + PSEMU_FLASH_SIZE);

  // The app returns to its own loop, where it reads the buttons, some frames after the undock.
  psemu_com_set_docked(m_ps, 0);
  for (u32 i = 0; i < UNDOCK_FRAMES; i++)
  {
    psemu_run(m_ps, FRAME_CYCLES);
    if (psemu_app_running(m_ps))
      break;
  }

  if (!psemu_app_running(m_ps))
  {
    WARNING_LOG("App did not resume after undocking, its progress was not saved.");
    return;
  }

  // Some apps save when undocked.
  if (std::memcmp(fl, flash_before.data(), PSEMU_FLASH_SIZE) != 0)
  {
    INFO_LOG("App saved its progress after undocking.");
    return;
  }

  // Only the action button. The direction buttons move through app menus, some of which hold actions that cannot be
  // undone, such as releasing a pet.
  psemu_set_buttons(m_ps, PSEMU_BUTTON_FIRE);
  u32 frames = 0;
  for (; frames < SAVE_BUTTON_HOLD_FRAMES; frames++)
  {
    psemu_run(m_ps, FRAME_CYCLES);
    if (std::memcmp(fl, flash_before.data(), PSEMU_FLASH_SIZE) != 0)
      break;
  }
  psemu_set_buttons(m_ps, 0);

  if (frames < SAVE_BUTTON_HOLD_FRAMES)
    INFO_LOG("App saved its progress after {} frames.", frames + 1);
  else
    INFO_LOG("App did not write to flash on exit.");
}

void PocketStation::EventCallback(void* param, TickCount ticks, TickCount ticks_late)
{
  static_cast<PocketStation*>(param)->Sync(true);
}

void PocketStation::Rebase(GlobalTicks now)
{
  m_anchor_ticks = now;
  m_units_run = 0;
  m_saved_valid = false;
}

void PocketStation::RunTo(GlobalTicks now)
{
  // The console clock goes backwards on reset, or when a save state loads. The device does not, so carry on from the
  // new time.
  if (now < m_anchor_ticks)
  {
    Rebase(now);
    return;
  }

  const u64 target = TicksToUnits(now - m_anchor_ticks);
  if (target <= m_units_run)
  {
    if (m_units_run - target > MAX_LEAD_UNITS)
      Rebase(now);
    return;
  }

  const u64 owed = target - m_units_run;
  if (owed > MAX_CATCH_UP_UNITS)
  {
    // A save state with a later time loaded. Skip the gap, as for a jump backwards, rather than stall emulation.
    DEV_LOG("Console clock jumped {} ms ahead, re-anchoring device time.", owed * 1000u / PSEMU_TIME_HZ);
    Rebase(now);
    return;
  }

  // Zero means the device cannot run at all. Count the time anyway, so the debt does not grow.
  const u64 ran = psemu_run_time(m_ps, owed);
  m_units_run += (ran != 0u) ? ran : owed;
}

void PocketStation::Sync(bool periodic)
{
  const GlobalTicks now = GetConsoleNow();
  RunTo(now);

  // Once the console passes the end of a run to an acknowledge, that run can no longer be undone.
  if (m_saved_valid && now >= m_anchor_ticks && TicksToUnits(now - m_anchor_ticks) >= m_ahead_until)
    m_saved_valid = false;

  if (periodic && CheckFlashWritten() && m_flash_written)
    m_flash_written();
}

void PocketStation::CatchUp()
{
  if (m_finished)
    return;

  const GlobalTicks now = GetConsoleNow();

  // If the device ran to an acknowledge past the time of this input, undo that run, and run to the input's time
  // instead. The device clock is exact, so this gives the same state as running straight to that time.
  if (m_saved_valid && now >= m_anchor_ticks)
  {
    const u64 target = TicksToUnits(now - m_anchor_ticks);
    if (m_units_run > target && m_saved_units <= target)
    {
      if (psemu_snapshot_load(m_ps, m_saved_state.data()) == PSEMU_OK)
        m_units_run = m_saved_units;
      else
        ERROR_LOG("Failed to load the state saved before the byte.");
    }
  }
  m_saved_valid = false;

  RunTo(now);
}

bool PocketStation::CheckFlashWritten()
{
  const u8* const fl = psemu_flash_data(m_ps);
  if (!fl || m_flash_snapshot.size() != PSEMU_FLASH_SIZE ||
      std::memcmp(fl, m_flash_snapshot.data(), PSEMU_FLASH_SIZE) == 0)
  {
    return false;
  }

  DEBUG_LOG("Flash written.");
  std::memcpy(m_flash_snapshot.data(), fl, PSEMU_FLASH_SIZE);
  return true;
}

void PocketStation::Select()
{
  CatchUp();
  psemu_com_set_selected(m_ps, 1);
}

bool PocketStation::Transfer(u8 data_in, u8* data_out)
{
  CatchUp();

  // The reply is what the kernel left in the output register, so it is known now. The acknowledge comes later, from
  // the kernel's interrupt handler, so save the device, and run it to the acknowledge to find its time. That run is
  // undone by CatchUp() if the console's next input comes first.
  *data_out = psemu_com_send(m_ps, data_in);
  psemu_snapshot_save(m_ps, m_saved_state.data());
  m_saved_valid = true;
  m_saved_units = m_units_run;

  m_units_run += psemu_run_time_to_ack(m_ps, ACK_BUDGET_UNITS);
  m_ahead_until = m_units_run;
  const bool acked = (psemu_com_acked(m_ps) != 0);

  m_ack_ticks = 0;
  if (acked)
  {
    const GlobalTicks ack_time = m_anchor_ticks + UnitsToTicksCeil(m_units_run);
    const GlobalTicks now = GetConsoleNow();
    m_ack_ticks = static_cast<TickCount>(std::max<GlobalTicks>(ack_time, now + 1) - now);
  }

  TRACE_LOG("Transfer in=0x{:02X} out=0x{:02X} ack={} ({} ticks)", data_in, *data_out, acked, m_ack_ticks);
  return acked;
}

void PocketStation::ResetTransferState(bool was_selected)
{
  if (!was_selected)
    return;

  // The kernel finishes the command in its own time after the release.
  CatchUp();

  if (psemu_cpu_faulted(m_ps))
    WARNING_LOG("Device CPU has faulted.");

  psemu_com_set_selected(m_ps, 0);
}

bool PocketStation::SaveFlash(MemoryCardImage::DataArray* data)
{
  // As of the current console time, not a run to an acknowledge that may yet be undone.
  CatchUp();

  MemoryCardImage::DataArray flash;
  if (psemu_save_flash_image(m_ps, flash.data(), flash.size()) != PSEMU_OK)
  {
    ERROR_LOG("Failed to read PocketStation flash.");
    return false;
  }

  if (flash == *data)
    return false;

  *data = flash;
  return true;
}
