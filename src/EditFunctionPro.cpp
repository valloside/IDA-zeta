#include "EditFunctionPro.h"

action_desc_t EditFunctionAction::getDescription(IDAZeta *mainPlugin) {
    action_desc_t actionDesc = {
        sizeof(action_desc_t), // 结构体大小
        ACTION_NAME,           // Action 的唯一内部名称
        NAME_IN_MENU,          // 在菜单中显示的名称
        this,                  // Action实例
        mainPlugin,            // 插件自身的指针
        nullptr,               // 快捷键
        TOOLTIP_IN_MENU,       // 悬停提示
        173                    // 一个内置的 IDA 图标
    };
    return actionDesc;
}

int idaapi EditFunctionAction::activate(action_activation_ctx_t *ctx) {
    static const char form[] =
        "Edit Function Pro\n"
        "\n"
        "\n"
        "<Frame local var size :N:1:::>\n"
        "<Saved register size :N:1:::>\n"
        "<Frame pointer delta :N:1:::>\n\n";

    func_t *func = get_func(ctx->cur_ea);
    uval_t  lvarSize = func->frsize;
    uval_t  savedRegSize = func->frregs;
    uval_t  newFpd = func->fpd;
    if (ask_form(form, &lvarSize, &savedRegSize, &newFpd) != 1)
        return 0;
    if (update_fpd(func, newFpd))
        msg("Frame pointer delta changed to: %d", newFpd);
    else
        msg("Fail to changed Frame pointer delta to: %d", newFpd);
    refresh_idaview_anyway();
    return 0;
}

action_state_t idaapi EditFunctionAction::update(action_update_ctx_t *ctx) {
    auto res = updateContext(ctx);
    if (res == AST_ENABLE)
        attach_action_to_popup(ctx->widget, nullptr, ACTION_NAME, "IDA ZETA/");
    else
        mCurrentWidget = BWN_UNKNOWN;
    return res;
}

action_state_t EditFunctionAction::updateContext(action_update_ctx_t *ctx) {
    if (ctx->widget_type != BWN_DISASM)
        return AST_DISABLE;
    flags64_t flags = get_flags(ctx->cur_ea);
    return is_code(flags) ? AST_ENABLE : AST_DISABLE;
}
