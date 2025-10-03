#pragma once

#include "dse/jet_command.hpp"

namespace dse::JetCommands {

inline const JetCommand DSERestoreAllDefaultParameters{DataType::kU32, "1011/03"};
inline const JetCommand DSESerialNumber{DataType::kU32, "4280/04"};
inline const JetCommand DSEIdentification{DataType::kAscii, "1008/00"};
inline const JetCommand DSEFirmwareVersion{DataType::kAscii, "100A/00"};
inline const JetCommand DSEZeroValue{DataType::kS32, "6142/00"};
inline const JetCommand CIA461ScaleFilter{DataType::kU16, "6040/01"};
inline const JetCommand DSELowPassCutOffFrequencyFIR{DataType::kU32, "3311/02"};
inline const JetCommand DSELowPassCutOffFrequencyIIR{DataType::kU32, "60A1/02"};

inline const JetCommand DSEFilterModeStage2{DataType::kU32, "6040/02"};
inline const JetCommand DSEFilterModeStage3{DataType::kU32, "6040/03"};
inline const JetCommand DSEFilterModeStage4{DataType::kU32, "6040/04"};
inline const JetCommand DSEFilterModeStage5{DataType::kU32, "6040/05"};

inline const JetCommand DSECombFilterFrequencyStage2{DataType::kU32, "3321/00"};
inline const JetCommand DSECombFilterFrequencyStage3{DataType::kU32, "3322/00"};
inline const JetCommand DSECombFilterFrequencyStage4{DataType::kU32, "3323/00"};
inline const JetCommand DSECombFilterFrequencyStage5{DataType::kU32, "3324/00"};

inline const JetCommand DSEMovAvFilterFrequencyStage2{DataType::kU32, "3331/00"};
inline const JetCommand DSEMovAvFilterFrequencyStage3{DataType::kU32, "3332/00"};
inline const JetCommand DSEMovAvFilterFrequencyStage4{DataType::kU32, "3333/00"};
inline const JetCommand DSEMovAvFilterFrequencyStage5{DataType::kU32, "3334/00"};

}  // namespace dse::JetCommands
