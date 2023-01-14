#include <SVGA/SDL.H>

#include <SVGA/LOGPHYS.H>
#include <SYSTEM/LOGPRINT.H>

#include <SDL2/SDL.h>
#include <assert.h>

// -----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
#define PAL_MAX_COLORS 256

// --- Private state -----------------------------------------------------------
SDL_Window*   sdlWindow   = NULL;
SDL_Surface*  sdlSurface  = NULL;
SDL_Palette*  sdlPalette  = NULL;
S32 screenLockCount = 0;

// --- Initialization ----------------------------------------------------------
S32 InitVideo() {
  assert(sdlWindow == NULL);
  assert(sdlSurface == NULL);

  if (SDL_VideoInit(NULL) != 0) {
    const char* errorMsg = SDL_GetError();
    LogPrintf("Error: Unable to initialize SDL Video subsystem.\n"
              "\tSDL Message: %s\n", errorMsg);

    return false;
  }

  return true;
}

void EndVideo() {
  if (sdlPalette) {
    SDL_FreePalette(sdlPalette);
    sdlPalette = NULL;
  }

  if (sdlWindow) {
    SDL_DestroyWindow(sdlWindow);
    sdlWindow = NULL;
    sdlSurface = NULL;
  }

  SDL_VideoQuit();
}

S32 CreateVideoSurface(U32 resX, U32 resY, U32 colorDepth) {
  assert(sdlWindow == NULL);
  assert(sdlSurface == NULL);
  assert(sdlPalette == NULL);

  // Create window
  sdlWindow = SDL_CreateWindow("LBA2 \"Twinsen's Odyssey\"",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               resX, resY,
                               0);
  if (sdlWindow == NULL) {
    const char* errorMsg = SDL_GetError();
    LogPrintf("Error: Unable to create SDL window.\n"
              "\tResolution: %ux%ux%u.\n"
              "\tSDL message: %s\n", resX, resY, colorDepth, errorMsg);

    EndVideo();

    return false;
  }

  // Create surface
  sdlSurface = SDL_GetWindowSurface(sdlWindow);
  if (sdlSurface == NULL) {
    const char* errorMsg = SDL_GetError();
    LogPrintf("Error: Unable to create SDL video surface.\n"
              "\tSDL message: %s\n", errorMsg);

    EndVideo();

    return false;
  }

  // Create palette
  sdlPalette = SDL_AllocPalette(PAL_MAX_COLORS);
  if (sdlPalette == NULL) {
    const char* errorMsg = SDL_GetError();
    LogPrintf("Error: Unable to create SDL palette.\n"
              "\tSDL message: %s\n", errorMsg);

    EndVideo();

    return false;
  }

  // Update global state
  ModeResX = resX; // Refers to Phys
  ModeResY = resY; // Refers to Phys
  BytesScanLine = resX; // SDL surface pitch may be bigger than requested
  Phys = malloc(resX * resY); // TODO Remove?

  return true;
}

// --- Draw --------------------------------------------------------------------
void LockVideoSurface() {
  assert(sdlSurface != NULL);

  if (screenLockCount == 0) {
    SDL_LockSurface(sdlSurface);
    ++screenLockCount;
  }
}

void UnlockVideoSurface() {
  assert(sdlSurface != NULL);

  if (screenLockCount > 0) {
    --screenLockCount;
  }

  if (screenLockCount == 0) {
    SDL_UnlockSurface(sdlSurface);
  }
}

void WaitVideoSync() {
  // TODO: Implement?
}

/**
 * @param src color sequence R G B, each color in palette requires 3 src entries
 * @param startIdx initial index in the color palette
 * @param count number of palette colors to update
 */
void SetVideoPalette(const U8 src[], S32 startIdx, S32 count) {
  assert(sdlPalette != NULL);

  static SDL_Color tmpColors[PAL_MAX_COLORS];

  // Sanitize input
  if (startIdx < 0) {
    startIdx = 0;
  }

  // Update temporary color table
  for (U32 i = startIdx; i < count; ++i) {
    tmpColors[i].r = src[0];
    tmpColors[i].g = src[1];
    tmpColors[i].b = src[2];
    tmpColors[i].a = 0xFF;
    src += 3;
  }

  // Update palette
  int ret = SDL_SetPaletteColors(sdlPalette, tmpColors, startIdx, count);
  if (ret != 0) {
    const char* errorMsg = SDL_GetError();
    LogPrintf("Warning: Unable to set SDL palette color.\n"
              "\tSDL message: %s\n", errorMsg);
  }
}

void SetVideoPaletteCol(S32 colorIdx, U8 r, U8 g, U8 b) {
  assert(sdlPalette != NULL);

  // Check input
  if ((colorIdx < 0) || (colorIdx >= PAL_MAX_COLORS)) {
    return;
  }

  SDL_Color tmpColor = {r, g, b, 0xFF};

  int ret = SDL_SetPaletteColors(sdlPalette, &tmpColor, colorIdx, 1);
  if (ret != 0) {
    const char* errorMsg = SDL_GetError();
    LogPrintf("Warning: Unable to set SDL palette color.\n"
              "\tSDL message: %s\n", errorMsg);
  }
}

void SetVideoPaletteSync(const U8 src[]) {
  SetVideoPalette(src, 0, PAL_MAX_COLORS);

  T_BOX copyArea = {0, 0, 640, 480};
  CopyVideoArea(Phys, Log, TabOffPhysLine, &copyArea);
}

// TODO: tabOffDst may need better name if used by src and dst
void CopyVideoArea(void* dst, const void* src, const U32 tabOffDst[],
                   const T_BOX* area) {
  assert(sdlWindow != NULL);
  assert(sdlSurface != NULL);
  assert(sdlPalette != NULL);

  U8* dstMem = (dst == Phys) ? (U8*)sdlSurface->pixels : (U8*)dst;
  U8* srcMem = (src == Phys) ? (U8*)sdlSurface->pixels : (U8*)src;
  U32 dstPitch = (dst == Phys) ? (U32)sdlSurface->pitch : tabOffDst[1];
  U32 srcPitch = (src == Phys) ? (U32)sdlSurface->pitch : tabOffDst[1];
  U8 dstBpp = (dst == Phys) ? sdlSurface->format->BytesPerPixel : 1;
  U8 srcBpp = (src == Phys) ? sdlSurface->format->BytesPerPixel : 1;

  if (src == Phys) {
    LogPuts("Warning: SDL Surface used as 'src' in CopyVideoArea"); // TODO Remove?
  }

  if (src == dst) {
    LogPuts("Warning: 'src' and 'dst' are the same. TODO: Investigate"); // TODO Remove?
  }

  if (dst == Phys) {
    LockVideoSurface();
  }

    for (int x = area->x0; x < area->x1; ++x) {
      for (int y = area->y0; y < area->y1; ++y) {

        // TODO Unify when using palette for SDL
        // TODO Make use of SDL Blit, must convert all buffers to SDL
        if (dst == Phys) {
          U8* srcPixel = (srcMem + (y * srcPitch) + (x * srcBpp)); // Src value is palette idx
          SDL_Color srcPaletteColor = sdlPalette->colors[*srcPixel];

          U32* dstPixel = (U32*)(dstMem + (y * dstPitch) + (x * dstBpp));
          *dstPixel = SDL_MapRGBA(sdlSurface->format,
                                  srcPaletteColor.r,
                                  srcPaletteColor.g,
                                  srcPaletteColor.b,
                                  srcPaletteColor.a);
        }
        else {
          U8* srcPixel = (srcMem + (y * srcPitch) + (x * srcBpp));
          U8* dstPixel = (dstMem + (y * dstPitch) + (x * dstBpp));

          *dstPixel = *srcPixel;
      }
    }

    if (dst == Phys) {
      UnlockVideoSurface();
    }

    if (dst == Phys) {
      SDL_Rect updateRect = {area->x0,
                             area->y0,
                             (area->x1 - area->x0),  // Width
                             (area->y1 - area->y0)}; // Height
      SDL_UpdateWindowSurface(sdlWindow);
      int ret = SDL_UpdateWindowSurfaceRects(sdlWindow, &updateRect, 1);
      if (ret != 0) {
          const char* errorMsg = SDL_GetError();
          LogPrintf("Warning: Unable to update SDL window surface area.\n"
                    "\tSDL message: %s\n", errorMsg);
      }
    }
  }
}

// =============================================================================
#ifdef __cplusplus
}
#endif
