// SPDX-FileCopyrightText: 2019-2024 Connor McLaughlin <stenzek@gmail.com>
// SPDX-FileCopyrightText: 2026 mentalfoundry
// SPDX-License-Identifier: GPL-3.0

#pragma once
#include "common/types.h"

class Error;

namespace PageFaultHandler {
enum class HandlerResult
{
  ContinueExecution,
  ExecuteNextHandler,
};

HandlerResult HandlePageFault(void* exception_pc, void* fault_address, bool is_write);
bool Install(Error* error = nullptr);

/// Uninstalls the handler, so that it does not outlive the module when PokketSX is unloaded as a plugin.
void Remove();
} // namespace PageFaultHandler
