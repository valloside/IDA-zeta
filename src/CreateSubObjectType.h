#pragma once

#include "CommonHeaders.h"

class IDAZeta;

class CreateSubObjectTypeAction : public action_handler_t {
public:
    struct member_info_t {
        uint64  offset;
        qstring name;
        bool    isBaseClass;
    };

    static constexpr const char *ACTION_NAME = "IDAZeta:CreateSubObjectAction";         // action 标识
    static constexpr const char *NAME_IN_MENU = "create sub object";                    // 右键菜单中的名称
    static constexpr const char *TOOLTIP_IN_MENU = "create a type contains sub object"; // 右键菜单中的悬浮提示
    static constexpr const char *SHORTCUT = nullptr;

    action_desc_t getDescription(IDAZeta *mainPlugin);

    virtual int idaapi activate(action_activation_ctx_t *ctx) override;

    virtual action_state_t idaapi update(action_update_ctx_t *ctx) override;

private:
    action_state_t _update(action_update_ctx_t *ctx);

    ssize_t showMemberChooser(const qvector<member_info_t> &members);

    qvector<member_info_t> getMemberInfoList(const tinfo_t &tif);

    tinfo_t createNewTypeStartFrom(const tinfo_t &originalTif, size_t startUdmIdx);
};