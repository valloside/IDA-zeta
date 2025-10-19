#include "CommonHeaders.h"
#include "FixSetTypeCrash.h"

static ssize_t idaapi uiCallbackSetTypeFix(void *ud, int notification_code, va_list va) {
    if (notification_code != ui_preprocess_action)
        return 0;
    const char *action_name = va_arg(va, const char *);
    if (action_name == nullptr || strcmp(action_name, "hx:SetType") != 0)
        return 0;
    vdui_t *vu = get_widget_vdui(get_current_viewer());
    if (vu == nullptr)
        return 0;
    if (vu->get_current_item(USE_KEYBOARD) && vu->item.citype == VDI_EXPR) {
        cexpr_t *expr = vu->item.e;
        if (expr != nullptr && expr->op == cot_obj) {
            ea_t       extern_ea = expr->obj_ea;
            segment_t *seg = getseg(extern_ea);
            flags64_t  flags = get_flags(extern_ea);
            if (seg != nullptr && seg->type == SEG_XTRN) {
                msg("[IDA-Zeta] action 'hx:SetType' on extern function may crash. Performing workaround...\n");
                jumpto(extern_ea);
                process_ui_action("SetType");
                process_ui_action("hx:JumpPseudo");
                return 1;
            }
        }
    }

    return 0;
}

void installCrashFix() {
    hook_to_notification_point(HT_UI, uiCallbackSetTypeFix, nullptr);
    msg("[IDA-Zeta] FixSetTypeCrash installed.\n");
}

void uninstallCrashFix() {
    unhook_from_notification_point(HT_UI, uiCallbackSetTypeFix, nullptr);
    msg("[IDA-Zeta] FixSetTypeCrash uninstalled.\n");
}