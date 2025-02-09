#pragma once

#include <memory>
#include <type_traits>

template<typename T>
struct is_smart_pointer : std::false_type
{};

template<typename T, typename D>
struct is_smart_pointer<std::unique_ptr<T, D>> : std::true_type
{};

template<typename T>
struct is_smart_pointer<std::shared_ptr<T>> : std::true_type
{};

template<typename T>
struct is_smart_pointer<std::weak_ptr<T>> : std::true_type
{};

template<typename T>
constexpr bool is_smart_pointer_v = is_smart_pointer<T>::value;

template<typename T>
constexpr bool is_pointer_or_smart_pointer_v = std::is_pointer_v<T> || is_smart_pointer_v<T>;

static_assert(!is_pointer_or_smart_pointer_v<int>);
static_assert(is_pointer_or_smart_pointer_v<int*>);
static_assert(is_pointer_or_smart_pointer_v<std::unique_ptr<int>>);
static_assert(is_pointer_or_smart_pointer_v<std::shared_ptr<int>>);
static_assert(is_pointer_or_smart_pointer_v<std::weak_ptr<int>>);
