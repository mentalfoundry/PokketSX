// SPDX-FileCopyrightText: 2019-2024 Connor McLaughlin <stenzek@gmail.com>
// SPDX-FileCopyrightText: 2026 mentalfoundry
// SPDX-License-Identifier: GPL-3.0

#include "core/achievements.h"
#include "core/controller.h"
#include "core/fullscreen_ui.h"
#include "core/game_list.h"
#include "core/gpu.h"
#include "core/host.h"
#include "core/settings.h"
#include "core/spu.h"
#include "core/system.h"

#include "scmversion/scmversion.h"

#include "util/audio_stream.h"
#include "util/gpu_device.h"
#include "util/imgui_fullscreen.h"
#include "util/imgui_manager.h"
#include "util/input_manager.h"
#include "util/platform_misc.h"

#include "common/crash_handler.h"
#include "common/error.h"
#include "common/file_system.h"
#include "common/log.h"
#include "common/memory_settings_interface.h"
#include "common/path.h"
#include "common/string_util.h"

#include "libretro.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#include "common/windows_headers.h"
#include <delayimp.h>
#endif

Log_SetChannel(LibretroHost);

#ifdef _WIN32

// RetroArch only searches its own folder for a core's dependencies, and has loaded some DLLs with the same names as
// ours, such as SDL2.dll. The project delay-loads our dependencies, and this loads each one from the core's folder.
static FARPROC WINAPI DelayLoadNotifyHook(unsigned notify, PDelayLoadInfo info)
{
  if (notify != dliNotePreLoadLibrary)
    return nullptr;

  HMODULE self;
  if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCWSTR>(&DelayLoadNotifyHook), &self))
  {
    return nullptr;
  }

  std::wstring path(MAX_PATH, L'\0');
  for (;;)
  {
    const DWORD len = GetModuleFileNameW(self, path.data(), static_cast<DWORD>(path.size()));
    if (len == 0)
      return nullptr;
    if (len < path.size())
    {
      path.resize(len);
      break;
    }
    path.resize(path.size() * 2);
  }

  path.erase(path.find_last_of(L"\\/") + 1);
  for (const char* ch = info->szDll; *ch != '\0'; ch++)
    path.push_back(static_cast<wchar_t>(*ch));

  // Altered search path, so that the DLL's own dependencies are also found next to it. If it isn't there, returning
  // null falls back to the normal search.
  return reinterpret_cast<FARPROC>(LoadLibraryExW(path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH));
}

extern "C" const PfnDliHook __pfnDliNotifyHook2 = DelayLoadNotifyHook;

#endif

namespace LibretroHost {

static constexpr u32 NUM_PORTS = 2;

// The display never exceeds VRAM at 1x. Hardware renderers multiply this by the resolution scale.
static constexpr u32 MAX_NATIVE_WIDTH = 1024;
static constexpr u32 MAX_NATIVE_HEIGHT = 512;

static constexpr unsigned RETRO_DEVICE_PS_CONTROLLER = RETRO_DEVICE_JOYPAD;
static constexpr unsigned RETRO_DEVICE_DUALSHOCK = RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 0);

class LibretroAudioStream final : public AudioStream
{
public:
  LibretroAudioStream(u32 sample_rate, const AudioStreamParameters& parameters);
  ~LibretroAudioStream() override;

  void Initialize();

  /// Sends every buffered frame to the frontend.
  void Flush();

private:
  std::vector<SampleType> m_output;
};

static void LogCallback(void* user_param, const char* channel_name, const char* function_name, LOGLEVEL level,
                        std::string_view message);
static void ShowMessage(std::string_view message);
static std::unique_ptr<AudioStream> CreateAudioStream(u32 sample_rate, const AudioStreamParameters& parameters);
static void InitializeSettings();
static bool SetFolders();
static void ApplyCoreOptions();
static void ApplyPortDevices();
static ControllerType GetControllerTypeForDevice(unsigned device);
static void UpdateControllers();
static void SendVideoFrame();
static void UpdateAVInfo(u32 width, u32 height);
static void GetSystemAVInfo(retro_system_av_info* info);

static retro_environment_t s_environment = nullptr;
static retro_video_refresh_t s_video_refresh = nullptr;
static retro_audio_sample_batch_t s_audio_sample_batch = nullptr;
static retro_input_poll_t s_input_poll = nullptr;
static retro_input_state_t s_input_state = nullptr;
static retro_log_printf_t s_log_printf = nullptr;

static std::unique_ptr<MemorySettingsInterface> s_base_settings_interface;
static LibretroAudioStream* s_audio_stream = nullptr;
static bool s_process_started = false;
static bool s_game_loaded = false;
static bool s_can_dupe = false;

static std::array<unsigned, NUM_PORTS> s_port_devices = {RETRO_DEVICE_PS_CONTROLLER, RETRO_DEVICE_PS_CONTROLLER};

static std::vector<u32> s_video_buffer;
static u32 s_last_video_width = 0;
static u32 s_last_video_height = 0;
static u32 s_geometry_width = 0;
static u32 s_geometry_height = 0;
static float s_geometry_aspect = 0.0f;
static u32 s_av_info_scale = 0;
static float s_av_info_fps = 0.0f;

static const retro_variable s_core_variables[] = {
  {"pokketsx_renderer", "Renderer; Software|Hardware"},
  {"pokketsx_resolution_scale", "Internal Resolution (hardware renderer); 1x|2x|3x|4x|5x|6x|8x"},
  {"pokketsx_region", "Console Region (restart); Auto|NTSC-U|NTSC-J|PAL"},
  {"pokketsx_fast_boot", "Fast Boot; disabled|enabled"},
  {"pokketsx_pocketstation_slot1", "PocketStation in Memory Card Slot 1; disabled|enabled"},
  {nullptr, nullptr},
};

static const retro_controller_description s_port_types[] = {
  {"PlayStation Controller", RETRO_DEVICE_PS_CONTROLLER},
  {"DualShock", RETRO_DEVICE_DUALSHOCK},
  {"None", RETRO_DEVICE_NONE},
};

static const retro_controller_info s_controller_info[] = {
  {s_port_types, static_cast<unsigned>(std::size(s_port_types))},
  {s_port_types, static_cast<unsigned>(std::size(s_port_types))},
  {nullptr, 0},
};

// Controller binding names, as declared by the digital and analog controllers, to RetroPad buttons.
struct ButtonMapping
{
  const char* name;
  unsigned retro_id;
};
static constexpr const ButtonMapping s_button_mappings[] = {
  {"Up", RETRO_DEVICE_ID_JOYPAD_UP},
  {"Right", RETRO_DEVICE_ID_JOYPAD_RIGHT},
  {"Down", RETRO_DEVICE_ID_JOYPAD_DOWN},
  {"Left", RETRO_DEVICE_ID_JOYPAD_LEFT},
  {"Triangle", RETRO_DEVICE_ID_JOYPAD_X},
  {"Circle", RETRO_DEVICE_ID_JOYPAD_A},
  {"Cross", RETRO_DEVICE_ID_JOYPAD_B},
  {"Square", RETRO_DEVICE_ID_JOYPAD_Y},
  {"Select", RETRO_DEVICE_ID_JOYPAD_SELECT},
  {"Start", RETRO_DEVICE_ID_JOYPAD_START},
  {"L1", RETRO_DEVICE_ID_JOYPAD_L},
  {"R1", RETRO_DEVICE_ID_JOYPAD_R},
  {"L2", RETRO_DEVICE_ID_JOYPAD_L2},
  {"R2", RETRO_DEVICE_ID_JOYPAD_R2},
  {"L3", RETRO_DEVICE_ID_JOYPAD_L3},
  {"R3", RETRO_DEVICE_ID_JOYPAD_R3},
};

// Analog controller half axes to RetroPad sticks.
struct HalfAxisMapping
{
  const char* name;
  unsigned retro_index;
  unsigned retro_id;
  bool negative;
};
static constexpr const HalfAxisMapping s_half_axis_mappings[] = {
  {"LLeft", RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, true},
  {"LRight", RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, false},
  {"LUp", RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, true},
  {"LDown", RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, false},
  {"RLeft", RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, true},
  {"RRight", RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, false},
  {"RUp", RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, true},
  {"RDown", RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, false},
};

} // namespace LibretroHost

LibretroHost::LibretroAudioStream::LibretroAudioStream(u32 sample_rate, const AudioStreamParameters& parameters)
  : AudioStream(sample_rate, parameters)
{
}

LibretroHost::LibretroAudioStream::~LibretroAudioStream()
{
  if (s_audio_stream == this)
    s_audio_stream = nullptr;
}

void LibretroHost::LibretroAudioStream::Initialize()
{
  BaseInitialize(&StereoSampleReaderImpl);
}

void LibretroHost::LibretroAudioStream::Flush()
{
  const u32 frames = GetBufferedFramesRelaxed();
  if (frames == 0)
    return;

  // Reading exactly what is buffered never underruns, so no silence is inserted.
  m_output.resize(frames * NUM_INPUT_CHANNELS);
  ReadFrames(m_output.data(), frames);

  // The frontend may take fewer frames than offered.
  const SampleType* data = m_output.data();
  u32 remaining = frames;
  while (remaining > 0)
  {
    const size_t written = s_audio_sample_batch(data, remaining);
    if (written == 0)
      break;

    data += written * NUM_INPUT_CHANNELS;
    remaining -= static_cast<u32>(written);
  }
}

std::unique_ptr<AudioStream> LibretroHost::CreateAudioStream(u32 sample_rate, const AudioStreamParameters& parameters)
{
  // The frontend owns timing and output, so the samples are passed on untouched.
  AudioStreamParameters stream_parameters = parameters;
  stream_parameters.expansion_mode = AudioExpansionMode::Disabled;
  stream_parameters.stretch_mode = AudioStretchMode::Off;

  std::unique_ptr<LibretroAudioStream> stream = std::make_unique<LibretroAudioStream>(sample_rate, stream_parameters);
  stream->Initialize();
  s_audio_stream = stream.get();
  return stream;
}

void LibretroHost::LogCallback(void* user_param, const char* channel_name, const char* function_name, LOGLEVEL level,
                               std::string_view message)
{
  if (!s_log_printf)
    return;

  retro_log_level retro_level;
  switch (level)
  {
    case LOGLEVEL_ERROR:
      retro_level = RETRO_LOG_ERROR;
      break;
    case LOGLEVEL_WARNING:
      retro_level = RETRO_LOG_WARN;
      break;
    case LOGLEVEL_INFO:
      retro_level = RETRO_LOG_INFO;
      break;
    default:
      retro_level = RETRO_LOG_DEBUG;
      break;
  }

  s_log_printf(retro_level, "[%s] %.*s\n", channel_name, static_cast<int>(message.size()), message.data());
}

void LibretroHost::ShowMessage(std::string_view message)
{
  const std::string message_str(message);
  const retro_message msg = {message_str.c_str(), 180};
  s_environment(RETRO_ENVIRONMENT_SET_MESSAGE, const_cast<retro_message*>(&msg));
}

void LibretroHost::InitializeSettings()
{
  // The base layer can only be set once per process, and the frontend may initialize the core again without
  // unloading it, so the same interface is kept and cleared.
  if (!s_base_settings_interface)
  {
    s_base_settings_interface = std::make_unique<MemorySettingsInterface>();
    Host::Internal::SetBaseSettingsLayer(s_base_settings_interface.get());
  }
  else
  {
    s_base_settings_interface->Clear();
  }

  SettingsInterface& si = *s_base_settings_interface.get();
  Settings().Save(si, false);

  // The frontend paces emulation, collects the audio and owns input devices.
  si.SetFloatValue("Main", "EmulationSpeed", 0.0f);
  si.SetBoolValue("Main", "RewindEnable", false);
  si.SetIntValue("Main", "RunaheadFrameCount", 0);
  si.SetBoolValue("Main", "SaveStateOnExit", false);
  si.SetBoolValue("Main", "ConfirmPowerOff", false);
  si.SetStringValue("Audio", "Backend", AudioStream::GetBackendName(AudioBackend::Null));
  si.SetBoolValue("Display", "VSync", false);
  si.SetBoolValue("Display", "ShowOSDMessages", false);
  si.SetBoolValue("Cheevos", "Enabled", false);
  si.SetBoolValue("Logging", "LogToConsole", false);
  si.SetBoolValue("Logging", "LogToFile", false);
  si.SetStringValue("Logging", "LogLevel", Settings::GetLogLevelName(LOGLEVEL_INFO));
  si.SetStringValue("ControllerPorts", "MultitapMode", Settings::GetMultitapModeName(MultitapMode::Disabled));

  for (u32 i = 0; i < static_cast<u32>(InputSourceType::Count); i++)
    si.SetBoolValue("InputSources", InputManager::InputSourceToString(static_cast<InputSourceType>(i)), false);
}

bool LibretroHost::SetFolders()
{
  const char* system_dir = nullptr;
  if (!s_environment(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) || !system_dir || system_dir[0] == '\0')
  {
    ERROR_LOG("The frontend did not provide a system directory.");
    return false;
  }

  const char* save_dir = nullptr;
  if (!s_environment(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir) || !save_dir || save_dir[0] == '\0')
    save_dir = nullptr;

  // BIOS images go directly in the system directory, as with other libretro cores. Everything else that PokketSX
  // keeps, including its resources, lives in a subdirectory.
  EmuFolders::AppRoot = Path::Canonicalize(Path::Combine(system_dir, "pokketsx"));
  EmuFolders::DataRoot = EmuFolders::AppRoot;
  EmuFolders::Resources = Path::Combine(EmuFolders::AppRoot, "resources");

  SettingsInterface& si = *s_base_settings_interface.get();
  si.SetStringValue("BIOS", "SearchDirectory", Path::Canonicalize(system_dir).c_str());
  if (save_dir)
    si.SetStringValue("MemoryCards", "Directory", Path::Canonicalize(save_dir).c_str());

  INFO_LOG("Data Directory: {}", EmuFolders::DataRoot);
  INFO_LOG("Resources Directory: {}", EmuFolders::Resources);

  // Otherwise crash dumps land in the frontend's working directory.
  CrashHandler::SetWriteDirectory(EmuFolders::DataRoot);

  if (!FileSystem::DirectoryExists(EmuFolders::Resources.c_str()))
  {
    ERROR_LOG("Resources directory '{}' is missing.", EmuFolders::Resources);
    ShowMessage(fmt::format("PokketSX resources are missing. Copy the resources folder to {}.", EmuFolders::Resources));
    return false;
  }

  EmuFolders::LoadConfig(si);
  EmuFolders::EnsureFoldersExist();
  return true;
}

void LibretroHost::ApplyCoreOptions()
{
  SettingsInterface& si = *s_base_settings_interface.get();

  const auto get_variable = [](const char* key) -> std::string_view {
    retro_variable var = {key, nullptr};
    if (!s_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || !var.value)
      return {};
    return var.value;
  };

  const std::string_view renderer = get_variable("pokketsx_renderer");
  si.SetStringValue("GPU", "Renderer",
                    Settings::GetRendererName((renderer == "Hardware") ? GPURenderer::Automatic : GPURenderer::Software));

  const std::string_view scale = get_variable("pokketsx_resolution_scale");
  si.SetIntValue("GPU", "ResolutionScale", StringUtil::FromChars<s32>(scale.substr(0, scale.find('x'))).value_or(1));

  const std::string_view region = get_variable("pokketsx_region");
  ConsoleRegion console_region = ConsoleRegion::Auto;
  if (region == "NTSC-U")
    console_region = ConsoleRegion::NTSC_U;
  else if (region == "NTSC-J")
    console_region = ConsoleRegion::NTSC_J;
  else if (region == "PAL")
    console_region = ConsoleRegion::PAL;
  si.SetStringValue("Console", "Region", Settings::GetConsoleRegionName(console_region));

  si.SetBoolValue("BIOS", "PatchFastBoot", get_variable("pokketsx_fast_boot") == "enabled");
  si.SetBoolValue("MemoryCards", "Card1PocketStation", get_variable("pokketsx_pocketstation_slot1") == "enabled");
}

ControllerType LibretroHost::GetControllerTypeForDevice(unsigned device)
{
  switch (device)
  {
    case RETRO_DEVICE_PS_CONTROLLER:
      return ControllerType::DigitalController;
    case RETRO_DEVICE_DUALSHOCK:
      return ControllerType::AnalogController;
    default:
      return ControllerType::None;
  }
}

void LibretroHost::ApplyPortDevices()
{
  SettingsInterface& si = *s_base_settings_interface.get();
  for (u32 port = 0; port < NUM_PORTS; port++)
  {
    const ControllerType type = GetControllerTypeForDevice(s_port_devices[port]);
    si.SetStringValue(TinyString::from_format("Pad{}", port + 1).c_str(), "Type",
                      Controller::GetControllerInfo(type)->name);
  }
}

void LibretroHost::UpdateControllers()
{
  s_input_poll();

  for (u32 port = 0; port < NUM_PORTS; port++)
  {
    Controller* const pad = System::GetController(port);
    if (!pad)
      continue;

    const Controller::ControllerInfo* const info = Controller::GetControllerInfo(pad->GetType());
    if (!info)
      continue;

    for (const Controller::ControllerBindingInfo& bi : info->bindings)
    {
      if (bi.type == InputBindingInfo::Type::Button)
      {
        const auto it = std::find_if(std::begin(s_button_mappings), std::end(s_button_mappings),
                                     [&bi](const ButtonMapping& bm) { return std::strcmp(bm.name, bi.name) == 0; });
        if (it == std::end(s_button_mappings))
          continue;

        const bool pressed = s_input_state(port, RETRO_DEVICE_JOYPAD, 0, it->retro_id) != 0;
        pad->SetBindState(bi.bind_index, pressed ? 1.0f : 0.0f);
      }
      else if (bi.type == InputBindingInfo::Type::HalfAxis)
      {
        const auto it =
          std::find_if(std::begin(s_half_axis_mappings), std::end(s_half_axis_mappings),
                       [&bi](const HalfAxisMapping& hm) { return std::strcmp(hm.name, bi.name) == 0; });
        if (it == std::end(s_half_axis_mappings))
          continue;

        const s32 raw = s_input_state(port, RETRO_DEVICE_ANALOG, it->retro_index, it->retro_id);
        const s32 directed = it->negative ? -raw : raw;
        pad->SetBindState(bi.bind_index, std::clamp(static_cast<float>(directed) / 32767.0f, 0.0f, 1.0f));
      }
    }
  }
}

void LibretroHost::SendVideoFrame()
{
  u32 width, height;
  if (!g_gpu || !g_gpu->ReadDisplayTextureRGBA8(&width, &height, &s_video_buffer))
  {
    // Nothing is displayed, e.g. the game has turned the display off.
    if (s_can_dupe && s_last_video_width != 0)
    {
      s_video_refresh(nullptr, s_last_video_width, s_last_video_height, 0);
      return;
    }

    width = (s_last_video_width != 0) ? s_last_video_width : 320;
    height = (s_last_video_height != 0) ? s_last_video_height : 240;
    s_video_buffer.assign(width * height, 0);
  }
  else
  {
    // RGBA8 in memory is 0xAABBGGRR, libretro's XRGB8888 is 0x00RRGGBB.
    for (u32& pixel : s_video_buffer)
      pixel = ((pixel & 0xFFu) << 16) | (pixel & 0xFF00u) | ((pixel >> 16) & 0xFFu);
  }

  UpdateAVInfo(width, height);

  s_video_refresh(s_video_buffer.data(), width, height, width * sizeof(u32));
  s_last_video_width = width;
  s_last_video_height = height;
}

void LibretroHost::GetSystemAVInfo(retro_system_av_info* info)
{
  const u32 scale = std::max<u32>(g_settings.gpu_resolution_scale, 1);
  const float aspect = g_gpu ? g_gpu->ComputeDisplayAspectRatio() : (4.0f / 3.0f);

  info->geometry.base_width = (s_last_video_width != 0) ? s_last_video_width : 320;
  info->geometry.base_height = (s_last_video_height != 0) ? s_last_video_height : 240;
  info->geometry.max_width = MAX_NATIVE_WIDTH * scale;
  info->geometry.max_height = MAX_NATIVE_HEIGHT * scale;
  info->geometry.aspect_ratio = aspect;
  info->timing.fps = System::IsValid() ? System::GetThrottleFrequency() : 60.0;
  info->timing.sample_rate = static_cast<double>(SPU::SAMPLE_RATE);

  s_av_info_scale = scale;
  s_av_info_fps = static_cast<float>(info->timing.fps);
  s_geometry_width = info->geometry.base_width;
  s_geometry_height = info->geometry.base_height;
  s_geometry_aspect = aspect;
}

void LibretroHost::UpdateAVInfo(u32 width, u32 height)
{
  // A new frame rate or maximum size needs the frontend to reinitialize, so only ask for that when it changes.
  const u32 scale = std::max<u32>(g_settings.gpu_resolution_scale, 1);
  if (scale != s_av_info_scale || System::GetThrottleFrequency() != s_av_info_fps)
  {
    s_last_video_width = width;
    s_last_video_height = height;

    retro_system_av_info info;
    GetSystemAVInfo(&info);
    s_environment(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
    return;
  }

  const float aspect = g_gpu->ComputeDisplayAspectRatio();
  if (width == s_geometry_width && height == s_geometry_height && aspect == s_geometry_aspect)
    return;

  retro_game_geometry geometry = {};
  geometry.base_width = width;
  geometry.base_height = height;
  geometry.max_width = MAX_NATIVE_WIDTH * scale;
  geometry.max_height = MAX_NATIVE_HEIGHT * scale;
  geometry.aspect_ratio = aspect;
  s_environment(RETRO_ENVIRONMENT_SET_GEOMETRY, &geometry);

  s_geometry_width = width;
  s_geometry_height = height;
  s_geometry_aspect = aspect;
}

//////////////////////////////////////////////////////////////////////////
// libretro API
//////////////////////////////////////////////////////////////////////////

RETRO_API void retro_set_environment(retro_environment_t cb)
{
  using namespace LibretroHost;

  s_environment = cb;

  retro_log_callback log = {};
  if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
    s_log_printf = log.log;

  cb(RETRO_ENVIRONMENT_SET_VARIABLES, const_cast<retro_variable*>(s_core_variables));
  cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, const_cast<retro_controller_info*>(s_controller_info));
}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb)
{
  LibretroHost::s_video_refresh = cb;
}

RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb)
{
  // Batched samples only.
}

RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
  LibretroHost::s_audio_sample_batch = cb;
}

RETRO_API void retro_set_input_poll(retro_input_poll_t cb)
{
  LibretroHost::s_input_poll = cb;
}

RETRO_API void retro_set_input_state(retro_input_state_t cb)
{
  LibretroHost::s_input_state = cb;
}

RETRO_API unsigned retro_api_version(void)
{
  return RETRO_API_VERSION;
}

RETRO_API void retro_init(void)
{
  using namespace LibretroHost;

  Log::RegisterCallback(&LogCallback, nullptr);
  InitializeSettings();

  Error error;
  if (!System::Internal::PerformEarlyHardwareChecks(&error) || !System::Internal::ProcessStartup(&error))
  {
    ERROR_LOG("Process startup failed: {}", error.GetDescription());
    return;
  }

  s_process_started = true;
}

RETRO_API void retro_deinit(void)
{
  using namespace LibretroHost;

  if (s_process_started)
  {
    System::Internal::ProcessShutdown();
    s_process_started = false;
  }

  Log::UnregisterCallback(&LogCallback, nullptr);
}

RETRO_API void retro_get_system_info(retro_system_info* info)
{
  std::memset(info, 0, sizeof(*info));
  info->library_name = "PokketSX";
  info->library_version = g_scm_tag_str;
  info->valid_extensions = "cue|bin|img|iso|chd|ecm|mds|pbp|m3u|exe|psexe|ps-exe|psf|minipsf";
  info->need_fullpath = true;
  info->block_extract = false;
}

RETRO_API void retro_get_system_av_info(retro_system_av_info* info)
{
  LibretroHost::GetSystemAVInfo(info);
}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device)
{
  using namespace LibretroHost;

  if (port >= NUM_PORTS)
    return;

  s_port_devices[port] = device;
  if (!s_base_settings_interface)
    return;

  ApplyPortDevices();
  if (System::IsValid())
    System::ApplySettings(false);
}

RETRO_API bool retro_load_game(const retro_game_info* game)
{
  using namespace LibretroHost;

  if (!s_process_started || !game || !game->path)
    return false;

  retro_pixel_format pixel_format = RETRO_PIXEL_FORMAT_XRGB8888;
  if (!s_environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format))
  {
    ERROR_LOG("The frontend does not support XRGB8888.");
    return false;
  }

  bool can_dupe = false;
  s_can_dupe = s_environment(RETRO_ENVIRONMENT_GET_CAN_DUPE, &can_dupe) && can_dupe;

  if (!SetFolders())
    return false;

  ApplyCoreOptions();
  ApplyPortDevices();

  ImGuiManager::SetFontPathAndRange(std::string(), {0x0020, 0x00FF, 0, 0});
  SPU::SetOutputStreamFactory(&CreateAudioStream);

  Error error;
  if (!System::Internal::CPUThreadInitialize(&error))
  {
    ERROR_LOG("CPUThreadInitialize() failed: {}", error.GetDescription());
    return false;
  }

  INFO_LOG("Booting '{}'...", game->path);
  if (!System::BootSystem(SystemBootParameters(game->path), &error))
  {
    ERROR_LOG("Failed to boot system: {}", error.GetDescription());
    ShowMessage(fmt::format("Failed to boot: {}", error.GetDescription()));
    System::Internal::CPUThreadShutdown();
    return false;
  }

  s_last_video_width = 0;
  s_last_video_height = 0;
  s_game_loaded = true;
  return true;
}

RETRO_API bool retro_load_game_special(unsigned game_type, const retro_game_info* info, size_t num_info)
{
  return false;
}

RETRO_API void retro_unload_game(void)
{
  using namespace LibretroHost;

  if (!s_game_loaded)
    return;

  System::ShutdownSystem(false);
  System::Internal::CPUThreadShutdown();
  SPU::SetOutputStreamFactory(nullptr);
  s_game_loaded = false;
}

RETRO_API void retro_run(void)
{
  using namespace LibretroHost;

  bool options_updated = false;
  if (s_environment(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &options_updated) && options_updated)
  {
    ApplyCoreOptions();
    System::ApplySettings(false);
  }

  UpdateControllers();

  System::ExecuteFrame();
  if (!System::IsValid())
  {
    s_environment(RETRO_ENVIRONMENT_SHUTDOWN, nullptr);
    return;
  }

  SendVideoFrame();

  if (s_audio_stream)
    s_audio_stream->Flush();
}

RETRO_API void retro_reset(void)
{
  if (System::IsValid())
    System::ResetSystem();
}

RETRO_API unsigned retro_get_region(void)
{
  return (System::IsValid() && System::IsPALRegion()) ? RETRO_REGION_PAL : RETRO_REGION_NTSC;
}

RETRO_API size_t retro_serialize_size(void)
{
  // TODO: Save states.
  return 0;
}

RETRO_API bool retro_serialize(void* data, size_t size)
{
  return false;
}

RETRO_API bool retro_unserialize(const void* data, size_t size)
{
  return false;
}

RETRO_API void retro_cheat_reset(void)
{
}

RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char* code)
{
}

RETRO_API void* retro_get_memory_data(unsigned id)
{
  return nullptr;
}

RETRO_API size_t retro_get_memory_size(unsigned id)
{
  return 0;
}

//////////////////////////////////////////////////////////////////////////
// Host interface
//////////////////////////////////////////////////////////////////////////

void Host::ReportFatalError(std::string_view title, std::string_view message)
{
  ERROR_LOG("ReportFatalError: {}: {}", title, message);
  LibretroHost::ShowMessage(message);
  System::ShutdownSystem(false);
}

void Host::ReportErrorAsync(std::string_view title, std::string_view message)
{
  if (!title.empty() && !message.empty())
    ERROR_LOG("ReportErrorAsync: {}: {}", title, message);
  else if (!message.empty())
    ERROR_LOG("ReportErrorAsync: {}", message);

  if (!message.empty())
    LibretroHost::ShowMessage(message);
}

bool Host::ConfirmMessage(std::string_view title, std::string_view message)
{
  if (!title.empty() && !message.empty())
    WARNING_LOG("ConfirmMessage: {}: {}", title, message);
  else if (!message.empty())
    WARNING_LOG("ConfirmMessage: {}", message);

  return true;
}

void Host::ReportDebuggerMessage(std::string_view message)
{
  ERROR_LOG("ReportDebuggerMessage: {}", message);
}

std::span<const std::pair<const char*, const char*>> Host::GetAvailableLanguageList()
{
  return {};
}

bool Host::ChangeLanguage(const char* new_language)
{
  return false;
}

s32 Host::Internal::GetTranslatedStringImpl(std::string_view context, std::string_view msg, char* tbuf,
                                            size_t tbuf_space)
{
  if (msg.size() > tbuf_space)
    return -1;
  else if (msg.empty())
    return 0;

  std::memcpy(tbuf, msg.data(), msg.size());
  return static_cast<s32>(msg.size());
}

std::string Host::TranslatePluralToString(const char* context, const char* msg, const char* disambiguation, int count)
{
  std::string ret(msg);
  StringUtil::ReplaceAll(&ret, "%n", fmt::format("{}", count));
  return ret;
}

SmallString Host::TranslatePluralToSmallString(const char* context, const char* msg, const char* disambiguation,
                                               int count)
{
  SmallString ret(msg);
  ret.replace("%n", TinyString::from_format("{}", count));
  return ret;
}

void Host::LoadSettings(SettingsInterface& si, std::unique_lock<std::mutex>& lock)
{
}

void Host::CheckForSettingsChanges(const Settings& old_settings)
{
}

void Host::CommitBaseSettingChanges()
{
  // noop, in memory
}

bool Host::ResourceFileExists(std::string_view filename, bool allow_override)
{
  const std::string path(Path::Combine(EmuFolders::Resources, filename));
  return FileSystem::FileExists(path.c_str());
}

std::optional<DynamicHeapArray<u8>> Host::ReadResourceFile(std::string_view filename, bool allow_override)
{
  const std::string path(Path::Combine(EmuFolders::Resources, filename));
  std::optional<DynamicHeapArray<u8>> ret(FileSystem::ReadBinaryFile(path.c_str()));
  if (!ret.has_value())
    ERROR_LOG("Failed to read resource file '{}'", filename);
  return ret;
}

std::optional<std::string> Host::ReadResourceFileToString(std::string_view filename, bool allow_override)
{
  const std::string path(Path::Combine(EmuFolders::Resources, filename));
  std::optional<std::string> ret(FileSystem::ReadFileToString(path.c_str()));
  if (!ret.has_value())
    ERROR_LOG("Failed to read resource file to string '{}'", filename);
  return ret;
}

std::optional<std::time_t> Host::GetResourceFileTimestamp(std::string_view filename, bool allow_override)
{
  const std::string path(Path::Combine(EmuFolders::Resources, filename));
  FILESYSTEM_STAT_DATA sd;
  if (!FileSystem::StatFile(path.c_str(), &sd))
  {
    ERROR_LOG("Failed to stat resource file '{}'", filename);
    return std::nullopt;
  }

  return sd.ModificationTime;
}

void Host::OnSystemStarting()
{
  //
}

void Host::OnSystemStarted()
{
  //
}

void Host::OnSystemDestroyed()
{
  //
}

void Host::OnSystemPaused()
{
  //
}

void Host::OnSystemResumed()
{
  //
}

void Host::OnIdleStateChanged()
{
  //
}

void Host::OnPerformanceCountersUpdated()
{
  //
}

void Host::OnGameChanged(const std::string& disc_path, const std::string& game_serial, const std::string& game_name)
{
  INFO_LOG("Disc Path: {}", disc_path);
  INFO_LOG("Game Serial: {}", game_serial);
  INFO_LOG("Game Name: {}", game_name);
}

void Host::OnMediaCaptureStarted()
{
  //
}

void Host::OnMediaCaptureStopped()
{
  //
}

void Host::PumpMessagesOnCPUThread()
{
  // System::ExecuteFrame() returns to retro_run() at the end of each frame, which handles everything.
}

void Host::RunOnCPUThread(std::function<void()> function, bool block /* = false */)
{
  // The frontend calls the core from one thread.
  function();
}

void Host::RequestResizeHostDisplay(s32 width, s32 height)
{
  //
}

void Host::RequestExitApplication(bool save_state_if_running)
{
  //
}

void Host::RequestExitBigPicture()
{
  //
}

void Host::RequestSystemShutdown(bool allow_confirm, bool save_state)
{
  System::ShutdownSystem(false);
}

bool Host::IsFullscreen()
{
  return false;
}

void Host::SetFullscreen(bool enabled)
{
  //
}

std::optional<WindowInfo> Host::AcquireRenderWindow(bool recreate_window)
{
  // Frames are read back and handed to the frontend, so there is never a window to present to.
  WindowInfo wi;
  wi.SetSurfaceless();
  return wi;
}

void Host::ReleaseRenderWindow()
{
  //
}

void Host::FrameDone()
{
  //
}

void Host::OpenURL(std::string_view url)
{
  //
}

bool Host::CopyTextToClipboard(std::string_view text)
{
  return false;
}

void Host::SetMouseMode(bool relative, bool hide_cursor)
{
  //
}

void Host::OnAchievementsLoginRequested(Achievements::LoginRequestReason reason)
{
  // noop
}

void Host::OnAchievementsLoginSuccess(const char* username, u32 points, u32 sc_points, u32 unread_messages)
{
  // noop
}

void Host::OnAchievementsRefreshed()
{
  // noop
}

void Host::OnAchievementsHardcoreModeChanged(bool enabled)
{
  // noop
}

void Host::OnCoverDownloaderOpenRequested()
{
  // noop
}

bool Host::ShouldPreferHostFileSelector()
{
  return false;
}

void Host::OpenHostFileSelectorAsync(std::string_view title, bool select_directory, FileSelectorCallback callback,
                                     FileSelectorFilters filters /* = FileSelectorFilters() */,
                                     std::string_view initial_directory /* = std::string_view() */)
{
  callback(std::string());
}

std::optional<u32> InputManager::ConvertHostKeyboardStringToCode(std::string_view str)
{
  return std::nullopt;
}

std::optional<std::string> InputManager::ConvertHostKeyboardCodeToString(u32 code)
{
  return std::nullopt;
}

const char* InputManager::ConvertHostKeyboardCodeToIcon(u32 code)
{
  return nullptr;
}

void Host::AddFixedInputBindings(SettingsInterface& si)
{
  // noop
}

void Host::OnInputDeviceConnected(std::string_view identifier, std::string_view device_name)
{
  // noop
}

void Host::OnInputDeviceDisconnected(InputBindingKey key, std::string_view identifier)
{
  // noop
}

std::optional<WindowInfo> Host::GetTopLevelWindowInfo()
{
  return std::nullopt;
}

void Host::RefreshGameListAsync(bool invalidate_cache)
{
  // noop
}

void Host::CancelGameListRefresh()
{
  // noop
}

BEGIN_HOTKEY_LIST(g_host_hotkeys)
END_HOTKEY_LIST()
