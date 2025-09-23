#pragma once

#pragma warning(push)
#pragma warning(disable : 4267) // warning C4267: “参数”: 从“size_t”转换到“int”，可能丢失数据
#pragma warning(disable : 4244) // warning C4244: “初始化”: 从“ssize_t”转换到“int”，可能丢失数据
#pragma warning(disable : 4018) // warning C4018: “<”: 有符号/无符号不匹配
#define _SILENCE_CXX20_IS_POD_DEPRECATION_WARNING

#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <hexrays.hpp>
#include <typeinf.hpp>
#include <frame.hpp>
#include <funcs.hpp>
#include <bytes.hpp>
#include <ua.hpp>
#include <moves.hpp>

#pragma warning(pop)