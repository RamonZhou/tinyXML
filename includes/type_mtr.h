#pragma once
#include <string>
#include <cstdlib>
#include <cxxabi.h>
#include <type_traits>
#include <memory>
#include <vector>
#include <queue>
#include <stack>
#include <deque>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

/**
 * @brief The type_mtr class
 *
 * These templates are used to distinguish different types
 */
namespace my_type_traits {

    // judge if it's a pair
    template <typename T>
    struct is_pair : std::false_type {};                        // if the following template are not matched, then return false
    template <typename T, typename U>
    struct is_pair<std::pair<T, U>> : std::true_type {};        // try to match pair
    template <typename T>
    inline constexpr bool is_pair_v = is_pair<T>::value;

    // judge if it's a container
    template <typename T, typename ... X>                       // the method is the same as matching pair
    struct is_container : std::false_type {};
    template <typename T, typename ... X>
    struct is_container<std::vector<T, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    struct is_container<std::queue<T, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    struct is_container<std::stack<T, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    struct is_container<std::deque<T, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    struct is_container<std::list<T, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    struct is_container<std::set<T, X ...>> : std::true_type {};
    template <typename T, typename U, typename ... X>
    struct is_container<std::map<T, U, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    struct is_container<std::multiset<T, X ...>> : std::true_type {};
    template <typename T, typename U, typename ... X>
    struct is_container<std::multimap<T, U, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    struct is_container<std::unordered_set<T, X ...>> : std::true_type {};
    template <typename T, typename U, typename ... X>
    struct is_container<std::unordered_map<T, U, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    inline constexpr bool is_container_v = is_container<T, X ...>::value;

    // judge if it's a container adaptor
    template <typename T, typename ... X>
    struct is_container_adaptor : std::false_type {};
    template <typename T, typename ... X>
    struct is_container_adaptor<std::queue<T, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    struct is_container_adaptor<std::stack<T, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    struct is_container_adaptor<std::priority_queue<T, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    inline constexpr bool is_container_adaptor_v = is_container_adaptor<T, X ...>::value;

    // judge if it's a sequence container
    template <typename T, typename ... X>
    struct is_sequence_container : std::false_type {};
    template <typename T, typename ... X>
    struct is_sequence_container<std::vector<T, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    struct is_sequence_container<std::deque<T, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    struct is_sequence_container<std::list<T, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    inline constexpr bool is_sequence_container_v = is_sequence_container<T, X ...>::value;

    // judge if it's a set
    template <typename T, typename ... X>
    struct is_set : std::false_type {};
    template <typename T, typename ... X>
    struct is_set<std::set<T, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    struct is_set<std::multiset<T, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    struct is_set<std::unordered_set<T, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    inline constexpr bool is_set_v = is_set<T, X ...>::value;

    // judge if it's a map
    template <typename T, typename ... X>
    struct is_map : std::false_type {};
    template <typename T, typename U, typename ... X>
    struct is_map<std::map<T, U, X ...>> : std::true_type {};
    template <typename T, typename U, typename ... X>
    struct is_map<std::multimap<T, U, X ...>> : std::true_type {};
    template <typename T, typename U, typename ... X>
    struct is_map<std::unordered_map<T, U, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    inline constexpr bool is_map_v = is_map<T, X ...>::value;

    // judge if it's a unique_ptr
    template <typename T, typename ... X>
    struct is_unique_ptr : std::false_type {};
    template <typename T, typename ... X>
    struct is_unique_ptr<std::unique_ptr<T, X ...>> : std::true_type {};
    template <typename T, typename ... X>
    inline constexpr bool is_unique_ptr_v = is_unique_ptr<T, X ...>::value;
}