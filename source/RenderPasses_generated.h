// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_RENDERPASSES_RENDERING_H_
#define FLATBUFFERS_GENERATED_RENDERPASSES_RENDERING_H_

#include "flatbuffers/flatbuffers.h"

namespace Rendering {

enum Pass {
  Pass_GBuffer = 0,
  Pass_Shadow = 1,
  Pass_Lighting = 2,
  Pass_PostProcess = 3,
  Pass_MIN = Pass_GBuffer,
  Pass_MAX = Pass_PostProcess
};

inline const Pass (&EnumValuesPass())[4] {
  static const Pass values[] = {
    Pass_GBuffer,
    Pass_Shadow,
    Pass_Lighting,
    Pass_PostProcess
  };
  return values;
}

inline const char * const *EnumNamesPass() {
  static const char * const names[] = {
    "GBuffer",
    "Shadow",
    "Lighting",
    "PostProcess",
    nullptr
  };
  return names;
}

inline const char *EnumNamePass(Pass e) {
  if (e < Pass_GBuffer || e > Pass_PostProcess) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesPass()[index];
}

inline const flatbuffers::TypeTable *PassTypeTable() {
  static const flatbuffers::TypeCode type_codes[] = {
    { flatbuffers::ET_CHAR, 0, 0 },
    { flatbuffers::ET_CHAR, 0, 0 },
    { flatbuffers::ET_CHAR, 0, 0 },
    { flatbuffers::ET_CHAR, 0, 0 }
  };
  static const flatbuffers::TypeFunction type_refs[] = {
    PassTypeTable
  };
  static const char * const names[] = {
    "GBuffer",
    "Shadow",
    "Lighting",
    "PostProcess"
  };
  static const flatbuffers::TypeTable tt = {
    flatbuffers::ST_ENUM, 4, type_codes, type_refs, nullptr, names
  };
  return &tt;
}

}  // namespace Rendering

#endif  // FLATBUFFERS_GENERATED_RENDERPASSES_RENDERING_H_