#define _CRT_SECURE_NO_WARNINGS
#include "AssignType.h"
#include "TypeNameParser.h"

// 检查光标下的反编译器元素是否可以设置类型
bool isAcceptableDecompilerItem(const ctree_item_t &item) {
    switch (item.citype) {
    case VDI_EXPR: // 1: 光标在变量求值表达式上 (例如 a = b + c; 中的 a 或 b 或 c)
        switch (item.it->op) {
        case cot_var: // 变量
            return true;
        case cot_obj: // 全局变量, 函数(TODO)
            return get_func(item.e->obj_ea) == nullptr;
        case cot_memref: // 成员操作 (例 expr.data)
        case cot_memptr: // 指针成员 (例 str->field)
            return true;
        case cot_call: // 函数调用 (TODO)
        default:
            return false;
        }
    case VDI_LVAR: // 2: 光标在局部变量声明上 (伪代码顶部的变量列表)
        return true;
    case VDI_FUNC: // 3: 光标在函数签名上
    default:
        return false;
    }
}

bool isMemOperand(insn_t &insn, int opnum) {
    if (opnum < 0 || opnum >= UA_MAXOP)
        return false;

    switch (insn.ops[opnum].type) {
    case o_mem:   // 直接内存引用 (全局变量)
    case o_displ: // 基址 + 偏移 (栈变量)
    case o_phrase:
    case o_far:
    case o_near:
        return true;
    default:
        return false;
    }
}

action_desc_t AssignTypeAction::getDescription(IDAZeta *mainPlugin) {
    action_desc_t actionDesc = {
        sizeof(action_desc_t), // 结构体大小
        ACTION_NAME,           // Action 的唯一内部名称
        NAME_IN_MENU,          // 在菜单中显示的名称
        this,                  // Action实例
        mainPlugin,            // 插件自身的指针
        SHORTCUT,              // 快捷键
        TOOLTIP_IN_MENU,       // 悬停提示
        174                    // 一个内置的 IDA 图标
    };
    return actionDesc;
}

int idaapi AssignTypeAction::activate(action_activation_ctx_t *ctx) {
    this->saveCurrentTypeInfo(ctx);

    if (!ask_str(&mTypeNameBuffer, HIST_TYPE, "Enter Type")) {
        msg("[IDA-Zeta] Assign type action cancelled.\n");
        return 0;
    }
    if (mTypeNameBuffer.empty()) {
        // 输入空，则尝试清除此变量的User-Defined数据，让其变成灰色的自动推断类型
        if (this->clearUserDefinedDataOfCurrentElem()) {
            msg("[IDA-Zeta] User-Defined data cleared.\n");
        }
        return 0;
    }

    tinfo_t tif;
    if (!parseDecoratedType(&tif, mTypeNameBuffer)) {
        return 0;
    }

    if (this->applyTypeInfo(tif)) {
        msg("[IDA-Zeta] Type '%s' applied successfully.\n", mTypeNameBuffer.c_str());
    } else {
        msg("[IDA-Zeta] Failed to apply type '%s' in the current context.\n", mTypeNameBuffer.c_str());
    }

    return 0;
}

action_state_t idaapi AssignTypeAction::update(action_update_ctx_t *ctx) {
    mCurrentWidget = ctx->widget_type;
    auto res = updateContext(ctx);
    if (res == AST_ENABLE)
        attach_action_to_popup(ctx->widget, nullptr, ACTION_NAME, "IDA ZETA/");
    else
        mCurrentWidget = BWN_UNKNOWN;
    return res;
}

action_state_t idaapi AssignTypeAction::updateContext(action_update_ctx_t *ctx) {
    switch (mCurrentWidget) {
    case BWN_PSEUDOCODE: { // 1: 在 Hex-Rays 反编译器窗口
        mVdui = get_widget_vdui(ctx->widget);
        if (mVdui == nullptr)
            break;

        if (isAcceptableDecompilerItem(mVdui->item)) {
            return AST_ENABLE;
        }
        break;
    }
    case BWN_DISASM: { // 2: IDA-View 反汇编窗口
        flags64_t flags = get_flags(ctx->cur_ea);
        if (is_data(flags)) // 光标在数据上
            return AST_ENABLE;

        if (!is_code(flags))
            break;
        insn_t insn;
        // 检查是否在汇编窗口的内存操作数上
        if (decode_insn(&insn, ctx->cur_ea) > 0) {
            // 使用全局函数 get_opnum 获取当前光标下的操作数
            if (isMemOperand(insn, get_opnum())) {
                return AST_ENABLE;
            }
        }
        break;
    }
    // case BWN_TICSR:  // 3: Local Types 窗口左半边的选择类型组件
    case BWN_TILIST: // 3: 类型列表，如 Local Types 窗口的右半边详情列表，以及 edit function prototype 对话
    case BWN_FRAME:  // 4: 栈帧/栈视图
        if (!ctx->type_ref)
            break;
        // idx == -1 in function is used for return type
        if (ctx->type_ref->memidx != -1)
            return AST_ENABLE;
        if (ctx->type_ref->is_func() && ctx->type_ref->memidx < (ssize_t)ctx->type_ref->nmembers)
            return AST_ENABLE;
    }
    // 默认情况下，禁用此功能
    return AST_DISABLE;
}

// 获取当前上下文中的类型字符串，以及其它信息
void AssignTypeAction::saveCurrentTypeInfo(action_activation_ctx_t *ctx) {
    this->resetSavedTypeInfo();
    const tinfo_t *currentTif = nullptr;
    switch (mCurrentWidget) {
    case BWN_PSEUDOCODE: {
        if (mVdui == nullptr)
            break;
        switch (mVdui->item.citype) {
        case VDI_EXPR: // 1: 光标在求值表达式上 (例如 a = b + c; 中的 a 或 b 或 c)
            switch (mVdui->item.it->op) {
                // 变量求值表达式
            case cot_var:
                // 取得声明处的类型，因为数组在表达式中求值会退化为指针类型
                mLocalVar = &mVdui->cfunc->get_lvars()->at(mVdui->item.e->v.idx);
                currentTif = &mLocalVar->tif;
                break;
            case cot_obj: { // 全局变量, 函数
                mDataEA = mVdui->item.e->obj_ea;
                get_func(mDataEA);
                tinfo_t tif;
                get_tinfo(&tif, mDataEA);
                tif.print(&mTypeNameBuffer, nullptr, PRTYPE_NOREGEX);
                return;
            }
            case cot_call: // 函数调用
                // TODO
                break;
            case cot_memref: { // 成员操作 (例如, expr.data)
                mFrameOrStructTif = &mVdui->item.e->x->type;
                udm_t udm;
                mMemberIndex = mFrameOrStructTif->get_udm_by_offset(&udm, mVdui->item.e->m * 8);
                currentTif = &mVdui->item.e->type;
                break;
            }
            case cot_memptr: { // 指针成员 (例如, str->field)
                mFrameOrStructTif = &mVdui->item.e->x->type;
                udm_t udm;
                mMemberIndex = mFrameOrStructTif->get_pointed_object().get_udm_by_offset(&udm, mVdui->item.e->m * 8);
                currentTif = &mVdui->item.e->type;
                break;
            }
            default:
                break;
            }
            break;
        case VDI_LVAR: // 局部变量 (声明)
            mLocalVar = mVdui->item.l;
            currentTif = &mLocalVar->tif;
            break;
        case VDI_FUNC:
            break;
        default:
            break;
        }
        break;
    }
    case BWN_DISASM: {
        flags64_t flags = get_flags(ctx->cur_ea);
        tinfo_t   tif;
        mDataEA = BADADDR;
        mFn = nullptr;
        if (is_data(flags)) { // 数据段
            mDataEA = ctx->cur_ea;
            if (!get_tinfo(&tif, ctx->cur_ea)) {
                if (is_strlit(flags)) {
                    size_t len = get_max_strlit_length(ctx->cur_ea, get_str_type(ctx->cur_ea));
                    tif.create_array(tinfo_t(BTF_CHAR), (uint32)len);
                } else if (is_float(flags)) {
                    tif = tinfo_t{BTMT_FLOAT};
                } else if (is_double(flags)) {
                    tif = tinfo_t{BTMT_DOUBLE};
                } else if (is_byte(flags)) {
                    tif = tinfo_t{BT_INT8};
                } else if (is_word(flags)) {
                    tif = tinfo_t{BT_INT16};
                } else if (is_dword(flags)) {
                    tif = tinfo_t{BT_INT32};
                } else if (is_qword(flags)) {
                    tif = tinfo_t{BT_INT64};
                } else if (is_oword(flags)) {
                    tif = tinfo_t{BT_INT128};
                } else {
                    break;
                }
            }
            tif.print(&mTypeNameBuffer, nullptr, PRTYPE_NOREGEX);
            break;
        }
        if (!is_code(flags)) // 代码段
            break;
        insn_t insn;
        if (decode_insn(&insn, ctx->cur_ea) > 0) {
            int   opnum = get_opnum();
            op_t &op = insn.ops[opnum];
            if (op.type == o_mem) { // 全局变量
                mDataEA = op.addr;
                get_tinfo(&tif, mDataEA);
                tif.print(&mTypeNameBuffer, nullptr, PRTYPE_NOREGEX);
            } else if (op.type == o_displ) { // 栈变量
                mFn = get_func(ctx->cur_ea);
                if (mFn == nullptr) break;

                mFrameVarOffset = calc_stkvar_struc_offset(mFn, insn, opnum);
                tinfo_t frame_tif;
                if (!get_func_frame(&frame_tif, mFn))
                    break;
                udm_t udm;
                if (frame_tif.get_udm_by_offset(&udm, mFrameVarOffset * 8) < 0)
                    break;

                udm.type.print(&mTypeNameBuffer, nullptr, PRTYPE_NOREGEX);
                return;
            }
        }
        break;
    }
    // case BWN_TICSR:  // 3: Local Types 窗口左半边的选择类型组件
    case BWN_TILIST: // 3: 类型列表，如 Local Types 窗口的右半边详情列表，以及 edit function prototype 对话
    case BWN_FRAME: {
        udm_t &udm = ctx->type_ref->udm;
        mFrameOrStructTif = &ctx->type_ref->tif;
        mMemberIndex = ctx->type_ref->memidx;
        mIsFunction = ctx->type_ref->is_func();
        const tinfo_t &tif = mIsFunction ? mFrameOrStructTif->get_nth_arg(static_cast<int>(mMemberIndex)) : udm.type;
        tif.print(&mTypeNameBuffer, nullptr, PRTYPE_NOREGEX);
        return;
    }
    }
    if (currentTif)
        currentTif->print(&mTypeNameBuffer, nullptr, PRTYPE_NOREGEX);
}

bool AssignTypeAction::applyTypeInfo(tinfo_t tif) const {
    switch (mCurrentWidget) {
    case BWN_PSEUDOCODE: // 1: 在 Hex-Rays 反编译器窗口
        if (mVdui == nullptr)
            break;
        if (mLocalVar && mVdui->set_lvar_type(mLocalVar, tif)) {
            mVdui->refresh_view(true);
            return true;
        } else if (mDataEA != BADADDR && set_tinfo(mDataEA, &tif)) {
            mVdui->refresh_view(true);
            return true;
        } else if (mFrameOrStructTif && mMemberIndex != -1) {
            tinfo_code_t code = mFrameOrStructTif->is_ptr_or_array()
                                  ? mFrameOrStructTif->get_pointed_object().set_udm_type(mMemberIndex, tif)
                                  : mFrameOrStructTif->set_udm_type(mMemberIndex, tif);
            if (code != tinfo_code_t::TERR_OK) {
                msg("[IDA-Zeta] Fail to set type of the member. reason: %s\n", tinfo_errstr(code));
                break;
            }
            mVdui->refresh_view(true);
            return true;
        }
        break;
    case BWN_DISASM:              // 2: IDA-View 反汇编窗口
        if (mDataEA != BADADDR) { // 全局变量
            if (set_tinfo(mDataEA, &tif)) {
                bool arr = tif.is_array();
                create_struct(mDataEA, tif.get_size(), arr ? tif.get_array_element().get_tid() : tif.get_tid());
                set_userti(mDataEA);
                refresh_idaview_anyway();
                return true;
            }
        } else if (mFn && set_frame_member_type(mFn, mFrameVarOffset, tif, nullptr, ETF_MAY_DESTROY)) { // 栈变量
            refresh_idaview_anyway();
            return true;
        }
        break;
    // case BWN_TICSR:  // 3: Local Types 窗口左半边的选择类型组件
    case BWN_TILIST:  // 3: 类型列表，如 Local Types 窗口的右半边详情列表，以及 edit function prototype 对话
    case BWN_FRAME: { // 4: 栈帧/栈视图
        auto code = tinfo_code_t::TERR_OK;
        if (mIsFunction) {
            if (mMemberIndex == -1)
                code = mFrameOrStructTif->set_func_rettype(tif, ETF_FUNCARG);
            else
                code = mFrameOrStructTif->set_funcarg_type(mMemberIndex, tif, ETF_FUNCARG);
        } else if (mCurrentWidget == BWN_FRAME) {
            code = mFrameOrStructTif->set_udm_type(mMemberIndex, tif, ETF_MAY_DESTROY);
        } else {
            code = mFrameOrStructTif->set_udm_type(mMemberIndex, tif);
        }
        if (code != tinfo_code_t::TERR_OK) {
            msg("[IDA-Zeta] Fail to set type of the member. reason: %s\n", tinfo_errstr(code));
            break;
        }
        refresh_idaview_anyway();
        return true;
    }
    }
    return false;
}

bool AssignTypeAction::clearUserDefinedDataOfCurrentElem() const {
    switch (mCurrentWidget) {
    case BWN_PSEUDOCODE:
        if (mLocalVar) {
            mVdui->set_lvar_type(mLocalVar, {});
            mVdui->refresh_view(true);
            return true;
        }
    case BWN_DISASM: {
        if (mDataEA != BADADDR) { // 全局变量
            clr_userti(mDataEA);
            refresh_idaview_anyway();
            return true;
        }
    }
    }
    return false;
}

void AssignTypeAction::resetSavedTypeInfo() {
    mFn = nullptr;
    mLocalVar = nullptr;
    mFrameVarOffset = 0;
    mTypeNameBuffer.clear();
    mDataEA = BADADDR;

    mFrameOrStructTif = nullptr;
    mMemberIndex = -1;
    mIsFunction = false;
}
