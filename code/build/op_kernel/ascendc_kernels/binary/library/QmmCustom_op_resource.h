#include <stdint.h>
#include <map>
#include <tuple>
#include <vector>
#include "graph/ascend_string.h"
#include "register/op_impl_registry.h"

extern gert::OpImplRegisterV2 op_impl_register_optiling_QmmCustom;
extern uint8_t _binary_config_ascend910b_qmm_custom_json_start;
extern uint8_t _binary_config_ascend910b_qmm_custom_json_end;
extern uint8_t _binary_ascend910b_qmm_custom_QmmCustom_38d1036ba9f412671b3a7b4a4969ed63_json_start;
extern uint8_t _binary_ascend910b_qmm_custom_QmmCustom_38d1036ba9f412671b3a7b4a4969ed63_json_end;
extern uint8_t _binary_ascend910b_qmm_custom_QmmCustom_38d1036ba9f412671b3a7b4a4969ed63_o_start;
extern uint8_t _binary_ascend910b_qmm_custom_QmmCustom_38d1036ba9f412671b3a7b4a4969ed63_o_end;
extern uint8_t _binary_ascend910b_qmm_custom_QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36_json_start;
extern uint8_t _binary_ascend910b_qmm_custom_QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36_json_end;
extern uint8_t _binary_ascend910b_qmm_custom_QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36_o_start;
extern uint8_t _binary_ascend910b_qmm_custom_QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36_o_end;

#define QmmCustom_OP_RESOURCES std::make_tuple<std::vector<void *>, \
    std::map<ge::AscendString, std::vector<std::tuple<const uint8_t *, const uint8_t *>>>, \
    std::vector<std::tuple<const uint8_t *, const uint8_t *>>>({nullptr, nullptr}, \
    { { "ascend910b", {    { &_binary_config_ascend910b_qmm_custom_json_start, \
      &_binary_config_ascend910b_qmm_custom_json_end } , \
            { &_binary_ascend910b_qmm_custom_QmmCustom_38d1036ba9f412671b3a7b4a4969ed63_json_start, \
      &_binary_ascend910b_qmm_custom_QmmCustom_38d1036ba9f412671b3a7b4a4969ed63_json_end } , \
            { &_binary_ascend910b_qmm_custom_QmmCustom_38d1036ba9f412671b3a7b4a4969ed63_o_start, \
      &_binary_ascend910b_qmm_custom_QmmCustom_38d1036ba9f412671b3a7b4a4969ed63_o_end } , \
            { &_binary_ascend910b_qmm_custom_QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36_json_start, \
      &_binary_ascend910b_qmm_custom_QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36_json_end } , \
            { &_binary_ascend910b_qmm_custom_QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36_o_start, \
      &_binary_ascend910b_qmm_custom_QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36_o_end }  } } }, \
    {  })

#define QmmCustom_RESOURCES {{"QmmCustom", QmmCustom_OP_RESOURCES}}