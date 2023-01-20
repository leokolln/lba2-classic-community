#pragma once

#include <SYSTEM/ADELINE_TYPES.H>
#include <SYSTEM/EVENTS.H> // For ManageEvents()

// -----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
extern volatile U32 TimerSystemHR;
extern volatile U32 TimerRefHR;
extern volatile S32 CmptFrame;        ///< Frame count
extern volatile S32 NbFramePerSecond; ///< FPS - Frames per second

// --- Initialization ----------------------------------------------------------
void InitTimer();
void EndTimer();

// -----------------------------------------------------------------------------
void LockTimer();
void UnlockTimer();
void SaveTimer();
void RestoreTimer();
void SetTimerHR(U32 time);

// -----------------------------------------------------------------------------
void ManageTime();

// -----------------------------------------------------------------------------
#define ManageSystem() { ManageEvents(); ManageTime(); }

// =============================================================================
#ifdef __cplusplus
}
#endif
