// SPDX-FileCopyrightText: Intel Corporation
//
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <iterator>
#include <ranges>
#include <type_traits>

namespace lib {

namespace ranges {

template <typename> inline constexpr bool disable_rank = false;

namespace {

template <typename T>
concept has_rank_method = requires(T t) {
                            { t.rank() } -> std::weakly_incrementable;
                          };

template <typename R>
concept has_rank_adl = requires(R &r) {
                         { rank_(r) } -> std::weakly_incrementable;
                       };

template <typename Iter>
concept is_remote_iterator_shadow_impl_ =
    std::forward_iterator<Iter> && has_rank_method<Iter> && !
disable_rank<std::remove_cv_t<Iter>>;

} // namespace

namespace {

struct rank_fn_ {

  // Return the rank associated with a remote range.
  // This is either:
  // 1) r.rank(), if the remote range has a `rank()` method
  // OR, if not available,
  // 2) r.begin().rank(), if iterator is `remote_iterator`
  template <std::ranges::forward_range R>
    requires((has_rank_method<R> && !disable_rank<std::remove_cv_t<R>>) ||
             (has_rank_adl<R> && !disable_rank<std::remove_cv_t<R>>) ||
             is_remote_iterator_shadow_impl_<std::ranges::iterator_t<R>>)
  constexpr auto operator()(R &&r) const {
    if constexpr (has_rank_method<R> && !disable_rank<std::remove_cv_t<R>>) {
      return std::forward<R>(r).rank();
    } else if constexpr (is_remote_iterator_shadow_impl_<
                             std::ranges::range_value_t<R>>) {
      return operator()(std::ranges::begin(std::forward<R>(r)));
    } else if constexpr (has_rank_adl<R> &&
                         !disable_rank<std::remove_cv_t<R>>) {
      return rank_(std::forward<R>(r));
    }
  }

  template <std::forward_iterator Iter>
    requires(has_rank_method<Iter> && !disable_rank<std::remove_cv_t<Iter>>)
  auto operator()(Iter iter) const {
    if constexpr (has_rank_method<Iter> &&
                  !disable_rank<std::remove_cv_t<Iter>>) {
      return std::forward<Iter>(iter).rank();
    }
  }
};

} // namespace

inline constexpr auto rank = rank_fn_{};

namespace {

template <typename R>
concept remote_range_shadow_impl_ =
    std::ranges::forward_range<R> && requires(R &r) { lib::ranges::rank(r); };

template <typename R>
concept segments_range =
    std::ranges::forward_range<R> &&
    remote_range_shadow_impl_<std::ranges::range_value_t<R>>;

template <typename R>
concept has_segments_method = requires(R r) {
                                { r.segments() } -> segments_range;
                              };

template <typename R>
concept has_segments_adl = requires(R &r) {
                             { segments_(r) } -> segments_range;
                           };

struct segments_fn_ {
  template <std::ranges::forward_range R>
    requires(has_segments_method<R> || has_segments_adl<R>)
  constexpr decltype(auto) operator()(R &&r) const {
    if constexpr (has_segments_method<R>) {
      return std::forward<R>(r).segments();
    } else if constexpr (has_segments_adl<R>) {
      return segments_(std::forward<R>(r));
    }
  }

  template <std::forward_iterator I>
    requires(has_segments_method<I> || has_segments_adl<I>)
  constexpr decltype(auto) operator()(I iter) const {
    if constexpr (has_segments_method<I>) {
      return std::forward<I>(iter).segments();
    } else if constexpr (has_segments_adl<I>) {
      return segments_(std::forward<I>(iter));
    }
  }
};

} // namespace

inline constexpr auto segments = segments_fn_{};

namespace {

template <typename Iter>
concept has_local_method =
    std::forward_iterator<Iter> && requires(Iter i) {
                                     { i.local() } -> std::forward_iterator;
                                   };

struct local_fn_ {

  template <std::forward_iterator Iter>
    requires(has_local_method<Iter> || std::contiguous_iterator<Iter>)
  auto operator()(Iter iter) const {
    if constexpr (has_local_method<Iter>) {
      return iter.local();
    } else if constexpr (std::contiguous_iterator<Iter>) {
      return iter;
    }
  }

  template <std::ranges::forward_range R>
    requires(has_local_method<std::ranges::iterator_t<R>> ||
             std::contiguous_iterator<std::ranges::iterator_t<R>> ||
             std::ranges::contiguous_range<R>)
  auto operator()(R &&r) const {
    if constexpr (has_local_method<std::ranges::iterator_t<R>>) {
      return std::span(std::ranges::begin(r).local(), std::ranges::size(r));
    } else if constexpr (std::contiguous_iterator<std::ranges::iterator_t<R>>) {
      return std::span(std::ranges::begin(r), std::ranges::size(r));
    }
  }
};

} // namespace

inline constexpr auto local = local_fn_{};

namespace {

template <typename Iter>
concept has_segment_index_method = requires(Iter i) {
                                     {
                                       i.segment_index()
                                       } -> std::weakly_incrementable;
                                   };

struct segment_index_ {

  template <std::forward_iterator Iter>
    requires(has_segment_index_method<Iter>)
  auto operator()(Iter iter) const {
    return iter.segment_index();
  }
};

} // namespace

inline constexpr auto segment_index = segment_index_{};

namespace {

template <typename Iter>
concept has_local_index_method = requires(Iter i) {
                                   {
                                     i.local_index()
                                     } -> std::weakly_incrementable;
                                 };

struct local_index_ {

  template <std::forward_iterator Iter>
    requires(has_local_index_method<Iter>)
  auto operator()(Iter iter) const {
    return iter.local_index();
  }
};

} // namespace

inline constexpr auto local_index = local_index_{};

} // namespace ranges

} // namespace lib
