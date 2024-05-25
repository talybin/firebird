#pragma once
#include <type_traits>

namespace fb
{

// Statically castable type
template <class F, class T,
    class = std::void_t<decltype(static_cast<T>(std::declval<F>()))>>
using static_castable = F;

// Concept to accept enum values
template <class F>
using index_castable = static_castable<F, size_t>;

} // namespace fb

