#pragma once

#include <SYSTEM/ADELINE_TYPES.H>

// -----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
#define TABKEYS_NUM_KEYS (256 + (16 * 8))

// --- Public state ------------------------------------------------------------
extern S32 AsciiMode;
extern S32 Key;
extern U8  TabKeys[TABKEYS_NUM_KEYS]; ///< Keyboard and Joysticks state

// --- Initialization ----------------------------------------------------------
void InitKeyboard();
void EndKeyboard();

// ---- Interface --------------------------------------------------------------
void ManageKeyboard();
S32 GetAscii();
S32 CheckKey(U32 key);

// =============================================================================
#ifdef __cplusplus
}
#endif
