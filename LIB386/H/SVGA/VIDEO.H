#pragma once

#include <SYSTEM/ADELINE_TYPES.H>

// -----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

// --- Initialization ----------------------------------------------------------
bool InitVideo();
void EndVideo();
bool CreateVideoSurface(U32 resX, U32 resY, U32 colorDepth);

// --- Draw --------------------------------------------------------------------
void LockVideoSurface();
void UnlockVideoSurface();
void WaitVideoSync();
void SetVideoPalette(const U8 src[], S32 startIdx, S32 count);
void SetVideoPaletteCol(S32 colorIdx, U8 r, U8 g, U8 b);
void SetVideoPaletteSync(const U8 src[]);
void CopyVideoArea(void* dst, const void* src, const U32 tabOffDst[],
                   const T_BOX* area);

// =============================================================================
#ifdef __cplusplus
}
#endif
