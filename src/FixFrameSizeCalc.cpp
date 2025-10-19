#include "FixFrameSizeCalc.h"

static ssize_t idaapi idp_callback(void *user_data, int notification_code, va_list va) {
    auto event = static_cast<pc_module_t::event_codes_t>(notification_code);
    switch (event) {
    case pc_module_t::ev_prolog_analyzed: {
        ea_t        first_past_prolog_insn = va_arg(va, ea_t);
        pushinfo_t *pushinfo = va_arg(va, pushinfo_t *);
        if (!pushinfo->psi.empty()
            && pushinfo->psi[0].off >= 0
            && pushinfo->psi[0].reg == RegNo::R_bp
            && pushinfo->bpidx == -1) {
            pushinfo->bpidx = 1;
        }
        break;
    }
    default:
        break;
    }
    return 0;
}

static ssize_t idaapi idb_callback(void *user_data, int notification_code, va_list va) {
    auto event = static_cast<pc_module_t::event_codes_t>(notification_code);
    switch (event) {
    case idb_event::loader_finished:
        unhook_from_notification_point(HT_IDP, idp_callback);
        break;
    default:
        break;
    }
    return 0;
}

FixFrameSizeCalc::FixFrameSizeCalc() {
    if (!hook_to_notification_point(HT_IDP, idp_callback, nullptr)) {
        msg("[FixFrameSize] C++ Processor Handler: failed to hook to HT_IDP.\n");
    }
    if (!hook_to_notification_point(HT_IDB, idb_callback, nullptr)) {
        msg("[FixFrameSize] C++ Processor Handler: failed to hook to HT_IDB.\n");
    }
}

FixFrameSizeCalc::~FixFrameSizeCalc() {
    unhook_from_notification_point(HT_IDP, idp_callback);
    unhook_from_notification_point(HT_IDB, idb_callback);
}
