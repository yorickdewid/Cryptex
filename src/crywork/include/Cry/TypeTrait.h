// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#pragma once

#include <Cry/Cry.h>

#include <stack>

namespace Cry
{
namespace TypeTrait
{

template<typename Type>
struct IsStack : public std::false_type {};

template<typename Type, typename Alloc>
struct IsStack<std::stack<Type, Alloc>> : public std::true_type {};

template<typename Type>
struct Identity { using type = typename Type::value_type; };

template<typename... ArgTypes>
struct TemplateHolder
{
    template <template <typename...> typename Wrapper>
    using template_apply = Wrapper<ArgTypes...>;
    constexpr static const auto size = sizeof...(ArgTypes);
};

} // namespace TypeTrait
} // namespace Cry