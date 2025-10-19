#pragma once

#include "CommonHeaders.h"

class IDAZeta;

class EditFunctionAction : public action_handler_t {
public:
    static constexpr const char *ACTION_NAME = "IDAZeta:EditFunctionAction";
    static constexpr const char *NAME_IN_MENU = "Edit Function";
    static constexpr const char *TOOLTIP_IN_MENU = "Edit Function";

    action_desc_t getDescription(IDAZeta *mainPlugin);

    virtual int idaapi activate(action_activation_ctx_t *ctx) override;

    virtual action_state_t idaapi update(action_update_ctx_t *ctx) override;

private:
    action_state_t updateContext(action_update_ctx_t *ctx);

    void saveCurrentTypeInfo(action_activation_ctx_t *ctx);

    bool applyTypeInfo(tinfo_t tif) const;

    bool clearUserDefinedDataOfCurrentElem() const;

    void resetSavedTypeInfo();

    // vvv Widget/window info vvvv

    twidget_type_t mCurrentWidget;
    vdui_t        *mVdui; // when Pseudocode window

    // vvv saved type info vvvv

    func_t *mFn;             // used by accessing stack var from Frame or Asm view
    lvar_t *mLocalVar;       // when Pseudocode window
    uval_t  mFrameVarOffset; // when Frame view. byte. relative to rsp
    ea_t    mDataEA;         // global data addr

    tinfo_t *mFrameOrStructTif;   // when Frame view, func arg list view or type list view. type for stack frame or a struct/union type
    ssize_t  mMemberIndex = -1;   // index of a stack var or struct/union member. used with mFrameOrStruct.
    bool     mIsFunction = false; // changing function type

    qstring mTypeNameBuffer;
};