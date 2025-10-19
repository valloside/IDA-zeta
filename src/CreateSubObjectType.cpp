#include "CreateSubObjectType.h"

using member_info_t = CreateSubObjectTypeAction::member_info_t;

action_desc_t CreateSubObjectTypeAction::getDescription(IDAZeta *mainPlugin) {
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

class member_chooser_t : public chooser_t {
public:
    member_chooser_t(const qvector<member_info_t> &members) :
        chooser_t(CH_MODAL | CH_KEEP, 2, WIDTHS, HEADERS, "Choose a member"), m_members(members) {
    }

    size_t idaapi get_count() const override { return m_members.size(); }

    void idaapi get_row(
        qstrvec_t            *out,
        int                  *out_icon,
        chooser_item_attrs_t *out_attrs,
        size_t                n
    ) const override {
        const member_info_t &mi = m_members[n];
        (*out)[0] = mi.name;
        (*out)[1].sprnt("%lX", mi.offset / 8);
    }

private:
    const qvector<member_info_t> &m_members;

    static constexpr int         WIDTHS[] = {60, 10};
    static constexpr const char *HEADERS[] = {"Member", "Offset"};
};

int CreateSubObjectTypeAction::activate(action_activation_ctx_t *ctx) {
    if (!ctx->type_ref)
        return 0;

    const tinfo_t &originalTif = ctx->type_ref->tif;

    qvector<member_info_t> members = getMemberInfoList(originalTif);

    if (members.empty()) {
        warning("Chosen type has no members to create a sub-type from.");
        return 0;
    }
    auto chosenMemberIndex = showMemberChooser(members);
    if (chosenMemberIndex < 0) {
        msg("User cancelled.");
        return 0;
    } else if (chosenMemberIndex == 0) {
        warning("You chose the first member, which means copy the type.");
        return 0;
    }

    tinfo_t newType = this->createNewTypeStartFrom(originalTif, chosenMemberIndex);
    if (newType.empty()) {
        warning("Failed to create sub-type.");
        return 0;
    }

    // Ask for a name for the new type
    qstring newTypeName;
    originalTif.get_type_name(&newTypeName);
    member_info_t &membInfo = members[chosenMemberIndex];
    if (membInfo.isBaseClass) {
        newTypeName.append("::baseclass_{").append(membInfo.name).append('}');
    } else {
        newTypeName.append("::subobj_{").append(membInfo.name).append('}');
    }

    if (!ask_str(&newTypeName, HIST_TYPE, "Enter name for the new sub-type")) {
        msg("User cancelled.");
        return 0;
    }

    if (newType.set_named_type(get_idati(), newTypeName.c_str()) != TERR_OK) {
        warning("Failed to add new type '%s' to local types.", newTypeName.c_str());
        return 0;
    }
    msg("[IDA-Zeta] Successfully created new sub-type '%s'.\n", newTypeName.c_str());

    uint32 new_ord = get_type_ordinal(get_idati(), newTypeName.c_str());
    if (new_ord != 0) {
        open_loctypes_window(new_ord);
    }

    return 1;
}

action_state_t CreateSubObjectTypeAction::update(action_update_ctx_t *ctx) {
    auto res = _update(ctx);
    if (res == AST_ENABLE)
        attach_action_to_popup(ctx->widget, nullptr, ACTION_NAME, "IDA ZETA/");
    return res;
}

action_state_t CreateSubObjectTypeAction::_update(action_update_ctx_t *ctx) {
    if (ctx->widget_type == BWN_TILIST
        && ctx->type_ref
        && ctx->type_ref->memidx == -1) {
        return AST_ENABLE;
    }
    return AST_DISABLE;
}

ssize_t CreateSubObjectTypeAction::showMemberChooser(const qvector<member_info_t> &members) {
    member_chooser_t ch(members);
    ssize_t          selection = ch.choose(1);
    return selection;
}

qvector<member_info_t> CreateSubObjectTypeAction::getMemberInfoList(const tinfo_t &tif) {
    if (!tif.is_udt())
        return {};

    udt_type_data_t udd;
    if (!tif.get_udt_details(&udd))
        return {};

    qvector<member_info_t> list;
    for (const auto &member : udd) {
        member_info_t mi;
        mi.offset = member.offset;
        if (mi.isBaseClass = member.is_baseclass()) {
            member.type.print(&mi.name);
        } else {
            mi.name = member.name;
        }
        list.push_back(mi);
    }
    return list;
}

tinfo_t CreateSubObjectTypeAction::createNewTypeStartFrom(const tinfo_t &tif, size_t startUdmIdx) {
    if (!tif.is_udt())
        return {};

    udt_type_data_t newUdt;
    udt_type_data_t original_udt;
    if (!tif.get_udt_details(&original_udt))
        return {};
    if (startUdmIdx >= original_udt.size()) {
        return {};
    }

    newUdt.sda = original_udt.sda;
    newUdt.pack = original_udt.pack;
    newUdt.taudt_bits = original_udt.taudt_bits;

    auto chosenMemberOffset = original_udt[startUdmIdx].offset;

    newUdt.reserve(original_udt.size() - startUdmIdx);
    for (size_t i = startUdmIdx; i < original_udt.size(); ++i) {
        newUdt.push_back(original_udt[i]);
        auto &new_member = newUdt.back();
        new_member.offset -= chosenMemberOffset;
    }

    tinfo_t newType;
    newType.create_udt(newUdt, BTF_STRUCT);
    return newType;
}
