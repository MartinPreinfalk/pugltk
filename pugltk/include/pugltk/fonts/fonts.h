#pragma once

#ifdef PUGLTK_EMBED_FONTS
#include "pugltk/fonts/cousine_regular_ttf.h"
#include "pugltk/fonts/droid_sans_ttf.h"
#include "pugltk/fonts/karla_regular_ttf.h"
#include "pugltk/fonts/proggy_clean_ttf.h"
#include "pugltk/fonts/proggy_tiny_ttf.h"
#include "pugltk/fonts/roboto_medium_ttf.h"
#endif

namespace pugltk::fonts {

constexpr size_t DefaultFontSize = 10;

enum class FontId {
  kDefault,
#ifdef PUGLTK_EMBED_FONTS
  kCousineRegular,
  kDroidSans,
  kKarlaRegular,
  kProggyClean,
  kProggyTiny,
  kRobotoMedium
#endif
};

#ifdef PUGLTK_EMBED_FONTS
constexpr FontId DefaultFontId = FontId::kKarlaRegular;
#else
constexpr FontId DefaultFontId = FontId::kDefault;
#endif


inline const void* GetCompressedTTFData(FontId const& font_id) {
#ifdef PUGLTK_EMBED_FONTS
  switch (font_id) {
    case FontId::kCousineRegular:
      return cousine_regular_ttf_compressed_data;
    case FontId::kDroidSans:
      return droid_sans_ttf_compressed_data;
    case FontId::kKarlaRegular:
      return karla_regular_ttf_compressed_data;
    case FontId::kProggyClean:
      return proggy_clean_ttf_compressed_data;
    case FontId::kProggyTiny:
      return proggy_tiny_ttf_compressed_data;
    case FontId::kRobotoMedium:
      return roboto_medium_ttf_compressed_data;
    default:
      break;
  }
#endif
  return nullptr;
}

inline int GetCompressedTTFSize(FontId const& font_id) {
#ifdef PUGLTK_EMBED_FONTS
  switch (font_id) {
    case FontId::kCousineRegular:
      return cousine_regular_ttf_compressed_size;
    case FontId::kDroidSans:
      return droid_sans_ttf_compressed_size;
    case FontId::kKarlaRegular:
      return karla_regular_ttf_compressed_size;
    case FontId::kProggyClean:
      return proggy_clean_ttf_compressed_size;
    case FontId::kProggyTiny:
      return proggy_tiny_ttf_compressed_size;
    case FontId::kRobotoMedium:
      return roboto_medium_ttf_compressed_size;
    default:
      break;
  }
#endif
  return 0;
}

}  // namespace pugltk::fonts