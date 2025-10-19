#include "SetAllFuncsUserDefined.h"

void SetAllFuncsToUserDefined::execute() {
    if (ask_yn(1, "Convert all functions to User-defined state? This will prevent the decompiler from automatically analyzing them.") != 1) {
        msg("[IDA-Zeta] Action canceled by user.\n");
        return;
    }

    size_t funcCount = get_func_qty();
    msg("[IDA-Zeta] Found %u functions. Starting conversion...\n", funcCount);

    size_t convertedCount = 0;
    for (size_t i = 0; i < funcCount; i++) {
        func_t *pFunc = getn_func(i);
        if (pFunc == nullptr) {
            continue;
        }

        tinfo_t funcType;
        if (!get_tinfo(&funcType, pFunc->start_ea)) {
            continue;
        }
        if (apply_tinfo(pFunc->start_ea, funcType, TINFO_DEFINITE)) {
            convertedCount++;
        }
    }
    refresh_idaview_anyway();
    msg("[IDA-Zeta] Conversion complete. %zu of %u functions were converted.\n", convertedCount, funcCount);
}