#pragma once

#include "CommonHeaders.h"
#include <vector>
#include <format>

namespace detail {

    constexpr inline bool isNameSeperator(char ch) {
        return ch == ' '
            || ch == '*' || ch == '&'
            || ch == '(' || ch == ')'
            || ch == '[' || ch == ']';
    }

    struct BaseTypeInfo {
        enum Modifier : char {
            None = 0,
            Const = 1,
            Volatile = 1 << 1,
            ConstVolatile = Const | Volatile
        };
        qstring  mName;
        Modifier mModifier = None;

        friend Modifier operator|=(Modifier &l, Modifier r) {
            l = Modifier((char)l | (char)r);
            return l;
        }
    };

    constexpr inline size_t ltrim(std::string_view &str) {
        for (size_t i = 0; i < str.size(); i++) {
            if (str[i] != ' ') {
                str = str.substr(i);
                return i;
            }
        }
        return 0;
    }

    struct TypeLayerInfo {
        enum kind_t { KIND_POINTER,
                      KIND_ARRAY };
        kind_t kind;

        asize_t arrayNElems = 0;
        bool    isConst = false;
        bool    isVolatile = false;
    };

    bool replace_base_type(
        const tinfo_t &originalType,
        tinfo_t       &newBaseType
    ) {
        std::vector<TypeLayerInfo> layers;
        tinfo_t                    currentType = originalType;
        tinfo_t                    oldBaseType;

        while (true) {
            TypeLayerInfo layer;
            if (currentType.is_array()) {
                layer.kind = TypeLayerInfo::KIND_ARRAY;
                layer.arrayNElems = currentType.get_array_nelems();
                layers.push_back(layer);

                currentType = currentType.get_array_element();
                continue;
            }
            if (currentType.is_ptr()) {
                layer.kind = TypeLayerInfo::KIND_POINTER;
                layer.isConst = currentType.is_const();
                layer.isVolatile = currentType.is_volatile();
                layers.push_back(layer);

                currentType = currentType.get_pointed_object();
                continue;
            }

            oldBaseType = currentType;
            break;
        }

        if (oldBaseType.is_const())
            newBaseType.set_const();
        if (oldBaseType.is_volatile())
            newBaseType.set_volatile();

        for (int i = (int)layers.size() - 1; i >= 0; --i) {
            const auto &layer = layers[i];
            tinfo_t     wrapper;

            switch (layer.kind) {
            case TypeLayerInfo::KIND_POINTER:
                wrapper.create_ptr(newBaseType);
                if (layer.isConst)
                    wrapper.set_const();
                if (layer.isVolatile)
                    wrapper.set_volatile();
                break;
            case TypeLayerInfo::KIND_ARRAY:
                wrapper.create_array(newBaseType, (uint32)layer.arrayNElems);
                break;
            }
            newBaseType = wrapper;
        }
        return true;
    }

} // namespace detail

// 解析带有CV修饰符、指针和数组的类型字符串
inline bool parseDecoratedType(tinfo_t *tif, const qstring &typeString) {
    // const T (*const (*)[3])[5]
    // ->
    //    base: T
    //    dummy: const char (*const (*)[3])[5]
    // -> parse ->
    //    tinfo_t: ptr -> arr[3] -> const ptr -> arr[5] -> const char
    // -> replace ->
    //    tinfo_t: ptr -> arr[3] -> const ptr -> arr[5] -> const T

    detail::BaseTypeInfo basetype;
    size_t               begin = 0;
    std::string_view     typeStringV{typeString.c_str(), typeString.size() - 1};
    begin += detail::ltrim(typeStringV);
    if (typeString.starts_with("const ")) {
        basetype.mModifier |= detail::BaseTypeInfo::Const;
        typeStringV = typeStringV.substr(6);
        begin += 6;
        begin += detail::ltrim(typeStringV);
        msg(std::format("[IDA-Zeta] const removed: {}\n", typeStringV).data());
    }
    if (typeStringV.starts_with("volatile ")) {
        basetype.mModifier |= detail::BaseTypeInfo::Volatile;
        typeStringV = typeStringV.substr(9);
        begin += 9;
        begin += detail::ltrim(typeStringV);
        msg(std::format("[IDA-Zeta] volatile removed: {}\n", typeStringV).data());
    }
    if (typeStringV.starts_with("const ")) {
        basetype.mModifier |= detail::BaseTypeInfo::ConstVolatile;
        typeStringV = typeStringV.substr(6);
        begin += 6;
        begin += detail::ltrim(typeStringV);
        msg(std::format("[IDA-Zeta] const removed: {}\n", typeStringV).data());
    }
    size_t level = 0, length = 0;
    for (auto c : typeStringV) {
        if (level == 0 && detail::isNameSeperator(c)) break;
        if (c == '<') ++level;
        if (c == '>') {
            if (level == 0)
                return false;
            --level;
        }

        length++;
    }
    basetype.mName = qstring{typeStringV.data(), length};
    msg("[IDA-Zeta] base type found: %s\n", basetype.mName.c_str());

    if (!tif->get_named_type(get_idati(), basetype.mName.c_str())) {
        basetype.mName.append(';');
        if (!parse_decl(tif, nullptr, get_idati(), basetype.mName.c_str(), PT_SIL | PT_TYP)) {
            basetype.mName.remove_last();
            msg("[IDA-Zeta] FATAL: Type parsing failed for \"%s\".\n", basetype.mName.c_str());
            return false;
        }
    }
    auto typeStringCpy = typeString;
    typeStringCpy.remove(begin, length);
    typeStringCpy.insert(begin, "char");
    typeStringCpy.append(';');
    msg("[IDA-Zeta] parse as dummy decl: %s\n", typeStringCpy.c_str());
    tinfo_t dummyTypeInfo;
    if (!parse_decl(&dummyTypeInfo, nullptr, get_idati(), typeStringCpy.c_str(), PT_SIL | PT_TYP | PT_VAR)) {
        msg("[IDA-Zeta] FATAL: type parse failed for \"%s\". (replaced \"%s\")\n", typeString.c_str(), typeStringCpy.c_str());
        return false;
    }
    detail::replace_base_type(dummyTypeInfo, *tif);

    return true;
}