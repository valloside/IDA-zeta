#include "CommonHeaders.h"
#include "AssignType.h"
#include "FixSetTypeCrash.h"
#include "SetAllFuncsUserdefined.h"
#include "EditFunctionPro.h"
#include "FixFrameSizeCalc.h"

class IDAZeta : public plugmod_t {
    enum InitStates {
        OK,
        Error
    };
    std::unique_ptr<AssignTypeAction>   mAssignType;
    std::unique_ptr<EditFunctionAction> mEditFunction;
    std::unique_ptr<FixFrameSizeCalc>   mFixFrameSizeCalc;
    InitStates                          mStates;

public:
    IDAZeta() :
        mAssignType(std::make_unique<AssignTypeAction>()),
        mEditFunction(std::make_unique<EditFunctionAction>()),
        mFixFrameSizeCalc(std::make_unique<FixFrameSizeCalc>()) {
        installCrashFix();
        action_desc_t actionDesc = mAssignType->getDescription(this);
        if (!register_action(actionDesc)) {
            msg("[IDA-Zeta] FATAL: Fail to register Action '%s'!\n", AssignTypeAction::ACTION_NAME);
            mStates = Error;
            return;
        }
        action_desc_t actionDesc2 = mEditFunction->getDescription(this);
        if (!register_action(actionDesc2)) {
            msg("[IDA-Zeta] FATAL: Fail to register Action '%s'!\n", EditFunctionAction::ACTION_NAME);
            mStates = Error;
            return;
        }
        mStates = OK;
        msg("[IDA-Zeta] Welcome to IDA-Zeta!!!\n");
    }

    ~IDAZeta() {
        uninstallCrashFix();
        unregister_action(AssignTypeAction::ACTION_NAME);
        unregister_action(EditFunctionAction::ACTION_NAME);
        msg("[IDA-Zeta] terminated!!!\n");
    }

    static plugmod_t *idaapi createInstance() {
        auto plug = new IDAZeta();
        if (!plug->isValid()) {
            delete plug;
            return PLUGIN_SKIP;
        }
        return plug;
    }

    bool isValid() const { return mStates == OK; }

    static int idaapi form_callback(int fid, form_actions_t &fa) {
        switch (fid) {
        case 1: {
            SetAllFuncsToUserDefined setFnUserDef;
            setFnUserDef.execute();
            break;
        }
        case 11: {
            info("IDA-Zeta\nGitHub: https://github.com/valloside/IDA-zeta");
            break;
        }
        default:
            break;
        }

        return 1;
    }

    bool idaapi run(size_t arg) override {
        static const char form[] =
            "功能面板\n"
            "\n"
            "\n"
            "<~S~et All function user-defined:B:1:::>\n"
            "\n"
            "<~A~bout:B:11:::>\n\n";

        ask_form(form, form_callback, form_callback, form_callback, form_callback, form_callback);
        return true;
    }
};

plugin_t PLUGIN = {
    IDP_INTERFACE_VERSION,
    PLUGIN_PROC | PLUGIN_MULTI, // flags
    IDAZeta::createInstance,    // init
    nullptr,                    // term
    nullptr,                    // run
    "IDA Zeta plugin",          // 状态栏的注释
    "C++ Reverse Helper",       // 帮助文本
    "IDA-Zeta",                 // 菜单名称
    nullptr                     // 快捷键
};