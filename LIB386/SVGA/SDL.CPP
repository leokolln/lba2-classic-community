#include <SVGA/SDL.H>

#include <SVGA/LOGPHYS.H>
#include <SYSTEM/LOGPRINT.H>
#include <SYSTEM/WINDOW.H>

#include <SDL2/SDL.h>
#include <assert.h>

// -----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
#define PAL_MAX_COLORS 256

// --- Private state -----------------------------------------------------------
SDL_Surface *sdlSurface = NULL;
SDL_Palette *sdlPalette = NULL;
S32 screenLockCount = 0;

// --- Initialization ----------------------------------------------------------
bool InitVideo() {
  assert(sdlSurface == NULL);

  return true;
}

void EndVideo() {
  if (sdlPalette) {
    SDL_FreePalette(sdlPalette);
    sdlPalette = NULL;
  }

  sdlSurface = NULL;
}

bool CreateVideoSurface(U32 resX, U32 resY, U32 colorDepth) {
  assert(sdlSurface == NULL);
  assert(sdlPalette == NULL);

  sdlSurface = (SDL_Surface *)(WindowSurface());
  if (sdlSurface == NULL) {
    return false;
  }

  // Create palette
  sdlPalette = SDL_AllocPalette(PAL_MAX_COLORS);
  if (sdlPalette == NULL) {
    const char *errorMsg = SDL_GetError();
    LogPrintf("Error: Unable to create SDL palette.\n"
              "\tSDL message: %s\n",
              errorMsg);

    return false;
  }

  // Update global state
  ModeResX = resX;      // Refers to Phys
  ModeResY = resY;      // Refers to Phys
  BytesScanLine = resX; // SDL surface pitch may be bigger than requested

  // TODO: Remove when blit supported? // FIXME: Must respect colorDepth bytes
  Phys = malloc(resX * resY);

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
    const char *errorMsg = SDL_GetError();
    LogPrintf("Warning: Unable to set SDL palette color.\n"
              "\tSDL message: %s\n",
              errorMsg);
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
    const char *errorMsg = SDL_GetError();
    LogPrintf("Warning: Unable to set SDL palette color.\n"
              "\tSDL message: %s\n",
              errorMsg);
  }
}

void SetVideoPaletteSync(const U8 src[]) {
  SetVideoPalette(src, 0, PAL_MAX_COLORS);

  T_BOX copyArea = {0, 0, 640, 480};
  CopyVideoArea(Phys, Log, TabOffPhysLine, &copyArea);
}

void CopyVideoArea(void *dst, const void *src, const U32 tabOffDst[],
                   const T_BOX *area) {
  assert(sdlSurface != NULL);
  assert(sdlPalette != NULL);
  assert((src != Phys) && "SDL Surface as 'src' is not supported");
  assert((src != dst) && "Copy to same video memory is not supported");

  U8 *dstMem = (dst == Phys) ? (U8 *)sdlSurface->pixels : (U8 *)dst;
  U8 *srcMem = (src == Phys) ? (U8 *)sdlSurface->pixels : (U8 *)src;
  U32 dstPitch = (dst == Phys) ? (U32)sdlSurface->pitch : tabOffDst[1];
  U32 srcPitch = (src == Phys) ? (U32)sdlSurface->pitch : tabOffDst[1];
  U8 dstBpp = (dst == Phys) ? sdlSurface->format->BytesPerPixel : 1;
  U8 srcBpp = (src == Phys) ? sdlSurface->format->BytesPerPixel : 1;

  if (dst == Phys) {
    LockVideoSurface();
  }

  for (int x = area->x0; x < area->x1; ++x) {
    for (int y = area->y0; y < area->y1; ++y) {

      // TODO Unify when using palette for SDL
      // TODO Make use of SDL Blit, must convert all buffers to SDL
      if (dst == Phys) { // Yes, I know... :D
        U8 *srcPixel = (srcMem + (y * srcPitch) + (x * srcBpp));
        SDL_Color srcPaletteColor = sdlPalette->colors[*srcPixel];

        U32 *dstPixel = (U32 *)(dstMem + (y * dstPitch) + (x * dstBpp));
        *dstPixel = SDL_MapRGBA(sdlSurface->format, srcPaletteColor.r,
                                srcPaletteColor.g, srcPaletteColor.b,
                                srcPaletteColor.a);
      } else {
        U8 *srcPixel = (srcMem + (y * srcPitch) + (x * srcBpp));
        U8 *dstPixel = (dstMem + (y * dstPitch) + (x * dstBpp));

        *dstPixel = *srcPixel;
      }
    }

    if (dst == Phys) {
      UnlockVideoSurface();
      UpdateWindowSurfaceArea(area, 1);
    }
  }
}

// =============================================================================
#ifdef __cplusplus
}
#endif
