#include "CommonHeaders.h"
#include "AssignType.h"
#include "FixSetTypeCrash.h"

class IDAZeta : public plugmod_t {
    enum InitStates {
        OK,
        Error
    };
    std::unique_ptr<AssignTypeAction> mAssignType;
    InitStates                        mStates;

public:
    IDAZeta() :
        mAssignType(std::make_unique<AssignTypeAction>()) {
        installCrashFix();
        action_desc_t actionDesc = mAssignType->getDescription(this);

        if (!register_action(actionDesc)) {
            msg("[IDA-Zeta] FATAL: Fail to register Action '%s'!\n", AssignTypeAction::ACTION_NAME);
            mStates = Error;
            return;
        }
        mStates = OK;
        msg("[IDA-Zeta] Welcome to IDA-Zeta!!!\n");
    }

    ~IDAZeta() {
        uninstallCrashFix();
        unregister_action(AssignTypeAction::ACTION_NAME);
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

    bool idaapi run(size_t arg) override {
        return true;
    }
};

plugin_t PLUGIN = {
    IDP_INTERFACE_VERSION,
    PLUGIN_MULTI,            // flags
    IDAZeta::createInstance, // init
    nullptr,                 // term
    nullptr,                 // run
    "IDA Zeta plugin",       // 状态栏的注释
    "C++ Reverse Helper",    // 帮助文本
    "IDA-Zeta",              // 菜单名称
    nullptr                  // 快捷键
};