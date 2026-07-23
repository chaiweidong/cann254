#include <stdint.h>
#include <map>
#include <tuple>
#include <vector>
#include "graph/ascend_string.h"
#include "register/op_bin_info.h"
#include "register/op_lib_register.h"
#include <dlfcn.h>
#include "../pkg_inc/base/dlog_pub.h"

extern uint8_t _binary_custom_op_impl_ai_core_tbe_config_ascend910b_aic_ascend910b_ops_info_json_start;
extern uint8_t _binary_custom_op_impl_ai_core_tbe_config_ascend910b_aic_ascend910b_ops_info_json_end;
extern uint8_t _binary_custom_op_impl_ai_core_tbe_kernel_config_ascend910b_binary_info_config_json_start;
extern uint8_t _binary_custom_op_impl_ai_core_tbe_kernel_config_ascend910b_binary_info_config_json_end;
extern uint8_t _binary_custom_op_impl_ai_core_tbe_kernel_config_ascend910b_qmm_custom_json_start;
extern uint8_t _binary_custom_op_impl_ai_core_tbe_kernel_config_ascend910b_qmm_custom_json_end;
extern uint8_t _binary_custom_op_impl_ai_core_tbe_kernel_ascend910b_qmm_custom_QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36_json_start;
extern uint8_t _binary_custom_op_impl_ai_core_tbe_kernel_ascend910b_qmm_custom_QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36_json_end;
extern uint8_t _binary_custom_op_impl_ai_core_tbe_kernel_ascend910b_qmm_custom_QmmCustom_38d1036ba9f412671b3a7b4a4969ed63_json_start;
extern uint8_t _binary_custom_op_impl_ai_core_tbe_kernel_ascend910b_qmm_custom_QmmCustom_38d1036ba9f412671b3a7b4a4969ed63_json_end;
extern uint8_t _binary_custom_op_impl_ai_core_tbe_kernel_ascend910b_qmm_custom_QmmCustom_38d1036ba9f412671b3a7b4a4969ed63_o_start;
extern uint8_t _binary_custom_op_impl_ai_core_tbe_kernel_ascend910b_qmm_custom_QmmCustom_38d1036ba9f412671b3a7b4a4969ed63_o_end;
extern uint8_t _binary_custom_op_impl_ai_core_tbe_kernel_ascend910b_qmm_custom_QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36_o_start;
extern uint8_t _binary_custom_op_impl_ai_core_tbe_kernel_ascend910b_qmm_custom_QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36_o_end;
extern uint8_t _binary_custom_op_impl_ai_core_tbe_custom_impl_dynamic_qmm_custom_py_start;
extern uint8_t _binary_custom_op_impl_ai_core_tbe_custom_impl_dynamic_qmm_custom_py_end;
extern uint8_t _binary_custom_op_impl_ai_core_tbe_custom_impl_dynamic_qmm_custom_cpp_start;
extern uint8_t _binary_custom_op_impl_ai_core_tbe_custom_impl_dynamic_qmm_custom_cpp_end;

#define ASCENDC_MODULE_NAME static_cast<int32_t>(ASCENDCKERNEL)
#define LOG_ERROR(format, ...)                                                         \
    do {                                                                               \
        dlog_error(ASCENDC_MODULE_NAME, "[%s] " format "\n", __FUNCTION__, ##__VA_ARGS__);  \
    } while (0)
namespace {
uint32_t OpLibInitFunc(ge::AscendString& op_lib_path) {
    static std::vector<std::tuple<ge::AscendString, ge::AscendString, const uint8_t *, const uint8_t *>> __ascendc_op_info_custom = 
{
 { "aic-ascend910b-ops-info.json", "custom/op_impl/ai_core/tbe/config/ascend910b", &_binary_custom_op_impl_ai_core_tbe_config_ascend910b_aic_ascend910b_ops_info_json_start, &_binary_custom_op_impl_ai_core_tbe_config_ascend910b_aic_ascend910b_ops_info_json_end}, 
 { "binary_info_config.json", "custom/op_impl/ai_core/tbe/kernel/config/ascend910b", &_binary_custom_op_impl_ai_core_tbe_kernel_config_ascend910b_binary_info_config_json_start, &_binary_custom_op_impl_ai_core_tbe_kernel_config_ascend910b_binary_info_config_json_end}, 
 { "qmm_custom.json", "custom/op_impl/ai_core/tbe/kernel/config/ascend910b", &_binary_custom_op_impl_ai_core_tbe_kernel_config_ascend910b_qmm_custom_json_start, &_binary_custom_op_impl_ai_core_tbe_kernel_config_ascend910b_qmm_custom_json_end}, 
 { "QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36.json", "custom/op_impl/ai_core/tbe/kernel/ascend910b/qmm_custom", &_binary_custom_op_impl_ai_core_tbe_kernel_ascend910b_qmm_custom_QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36_json_start, &_binary_custom_op_impl_ai_core_tbe_kernel_ascend910b_qmm_custom_QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36_json_end}, 
 { "QmmCustom_38d1036ba9f412671b3a7b4a4969ed63.json", "custom/op_impl/ai_core/tbe/kernel/ascend910b/qmm_custom", &_binary_custom_op_impl_ai_core_tbe_kernel_ascend910b_qmm_custom_QmmCustom_38d1036ba9f412671b3a7b4a4969ed63_json_start, &_binary_custom_op_impl_ai_core_tbe_kernel_ascend910b_qmm_custom_QmmCustom_38d1036ba9f412671b3a7b4a4969ed63_json_end}, 
 { "QmmCustom_38d1036ba9f412671b3a7b4a4969ed63.o", "custom/op_impl/ai_core/tbe/kernel/ascend910b/qmm_custom", &_binary_custom_op_impl_ai_core_tbe_kernel_ascend910b_qmm_custom_QmmCustom_38d1036ba9f412671b3a7b4a4969ed63_o_start, &_binary_custom_op_impl_ai_core_tbe_kernel_ascend910b_qmm_custom_QmmCustom_38d1036ba9f412671b3a7b4a4969ed63_o_end}, 
 { "QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36.o", "custom/op_impl/ai_core/tbe/kernel/ascend910b/qmm_custom", &_binary_custom_op_impl_ai_core_tbe_kernel_ascend910b_qmm_custom_QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36_o_start, &_binary_custom_op_impl_ai_core_tbe_kernel_ascend910b_qmm_custom_QmmCustom_a85df395ac5d0c4f7fc9465ac1e8ab36_o_end}, 
 { "qmm_custom.py", "custom/op_impl/ai_core/tbe/custom_impl/dynamic", &_binary_custom_op_impl_ai_core_tbe_custom_impl_dynamic_qmm_custom_py_start, &_binary_custom_op_impl_ai_core_tbe_custom_impl_dynamic_qmm_custom_py_end}, 
 { "qmm_custom.cpp", "custom/op_impl/ai_core/tbe/custom_impl/dynamic", &_binary_custom_op_impl_ai_core_tbe_custom_impl_dynamic_qmm_custom_cpp_start, &_binary_custom_op_impl_ai_core_tbe_custom_impl_dynamic_qmm_custom_cpp_end}, 
}; 
    static ops::OpBinInfo g_binInfo("custom", __ascendc_op_info_custom);
Dl_info dlInfo;
if (!dladdr((void*)&OpLibInitFunc, &dlInfo)) {
    LOG_ERROR("dladdr failed: %s", dlerror());
    return 1;
}
std::string targetPath = dlInfo.dli_fname;
if (!ops::OpBinInfo::Check(targetPath)) {
    LOG_ERROR("Path %s only support shared library, but it is not.",targetPath.c_str());
    return 1;
}
    return g_binInfo.Generate(&op_lib_path, targetPath);
}
REGISTER_OP_LIB(custom).RegOpLibInit(OpLibInitFunc);
}
