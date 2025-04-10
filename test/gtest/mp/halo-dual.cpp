// SPDX-FileCopyrightText: Intel Corporation
//
// SPDX-License-Identifier: BSD-3-Clause

#include "xp-tests.hpp"

template <typename T> class HaloDual : public testing::Test {};

TYPED_TEST_SUITE(HaloDual, ::testing::Types<dr::mp::dual_distributed_vector<int>>);

template <typename DV>
void check_matching(DV &dv, int idx, int expected_value) {
  typename DV::value_type *local_ptr = (dv.begin() + idx).local();
  EXPECT_TRUE(local_ptr != nullptr);
  typename DV::value_type value_on_host;

  if (dr::mp::use_sycl())
    dr::mp::__detail::sycl_copy(local_ptr, &value_on_host);
  else
    value_on_host = *local_ptr;

  DRLOG("checking idx:{} expected:{}", idx, expected_value);
  EXPECT_EQ(value_on_host, expected_value);
}

template <typename DV>
void local_is_accessible_in_halo_region(const int halo_prev,
                                        const int halo_next) {

  DV dv(12, dr::mp::distribution().halo(halo_prev, halo_next));
  DRLOG("local_is_accessible_in_halo_region TEST START, prev:{}, next:{}",
        halo_prev, halo_next);
  iota(dv, 0);
  DRLOG("exchange start");

  dv.halo().exchange();

  // arrays below is function depending on size of communicator-1
  std::array<int, 4> first_local_index____;
  std::array<int, 4> first_nonlocal_index_;
  std::array<int, 4> second_local_index___;
  std::array<int, 4> second_nonlocal_index;
  const int X = 10000; // to mark unused value

  switch (dr::mp::default_comm().rank()) {
  case 0:
    first_local_index____ = {+0, +0, +0, 0};
    first_nonlocal_index_ = {12, +3, +2, 2};
    second_local_index___ = {+X, +9, 10, X};
    second_nonlocal_index = {+X, 12, 12, X};
    break;
  case 1:
    first_local_index____ = {X, 3, +2, 2};
    first_nonlocal_index_ = {X, 9, +4, 4};
    second_local_index___ = {X, X, +8, X};
    second_nonlocal_index = {X, X, 10, X};
    break;
  case 2:
    first_local_index____ = {X, X, 4, +4};
    first_nonlocal_index_ = {X, X, 8, +6};
    second_local_index___ = {X, X, X, 10};
    second_nonlocal_index = {X, X, X, 12};
    break;
  case 3:
    first_local_index____ = {X, X, X, +6};
    first_nonlocal_index_ = {X, X, X, 10};
    second_local_index___ = {X, X, X, +X};
    second_nonlocal_index = {X, X, X, +X};
    break;
  default:
    first_local_index____ = {X, X, X, X};
    first_nonlocal_index_ = {X, X, X, X};
    second_local_index___ = {X, X, X, X};
    second_nonlocal_index = {X, X, X, X};
  }

  const auto c = dr::mp::default_comm().size() - 1;
  auto first_legal_idx = std::max(0, first_local_index____[c] - halo_prev);
  auto first_illegal_idx = std::min(12, first_nonlocal_index_[c] + halo_next);
  auto second_legal_idx = std::max(0, second_local_index___[c] - halo_prev);
  auto second_illegal_idx = std::min(12, second_nonlocal_index[c] + halo_next);

  DRLOG("checking access to idx between first legal {} and first illegal {}, "
        "c:{}",
        first_legal_idx, first_illegal_idx, c);

  for (int idx = first_legal_idx; idx < first_illegal_idx; ++idx) {
    check_matching(dv, idx, idx);
  }

  DRLOG("checking access to idx between second legal {} and second illegal {}, "
    "c:{}",
    second_legal_idx, second_illegal_idx, c);

  for (int idx = second_legal_idx; idx < second_illegal_idx; ++idx) {
    check_matching(dv, idx, idx);
  }

  DRLOG("checks ok");
}

TYPED_TEST(HaloDual, local_is_accessible_in_halo_region_halo_11) {
  local_is_accessible_in_halo_region<TypeParam>(1, 1);
}

TYPED_TEST(HaloDual, local_is_accessible_in_halo_region_halo_10) {
  local_is_accessible_in_halo_region<TypeParam>(1, 0);
}

TYPED_TEST(HaloDual, local_is_accessible_in_halo_region_halo_01) {
  local_is_accessible_in_halo_region<TypeParam>(0, 1);
}

template <typename DV>
void local_is_accessible_in_halo_region__partial(const int halo_prev,
                                                 const int halo_next) {

  DV dv(12, dr::mp::distribution().halo(halo_prev, halo_next));
  DRLOG("local_is_accessible_in_halo_region TEST START, prev:{}, next:{}",
        halo_prev, halo_next);
  iota(dv, 0);
  DRLOG("exchange start");

  dv.halo().exchange();

  // arrays below is function depending on size of communicator-1
  std::array<int, 4> first_segment_begin_;
  std::array<int, 4> first_segment_end___;
  std::array<int, 4> second_segment_begin;
  std::array<int, 4> second_segment_end__;
  const int X = 10000; // to mark unused value

  switch (dr::mp::default_comm().rank()) {
  case 0:
    first_segment_begin_ = {+0, +0, +0, 0};
    first_segment_end___ = {+6, +3, +2, 2};
    second_segment_begin = {+6, +9, 10, X};
    second_segment_end__ = {12, 12, 12, X};
    break;
  case 1:
    first_segment_begin_ = {X, 3, +2, 2};
    first_segment_end___ = {X, 6, +4, 4};
    second_segment_begin = {X, 6, +8, X};
    second_segment_end__ = {X, 9, 10, X};
    break;
  case 2:
    first_segment_begin_ = {X, X, 4, +4};
    first_segment_end___ = {X, X, 6, +6};
    second_segment_begin = {X, X, 6, 10};
    second_segment_end__ = {X, X, 8, 12};
    break;
  case 3:
    first_segment_begin_ = {X, X, X, +6};
    first_segment_end___ = {X, X, X, +8};
    second_segment_begin = {X, X, X, +8};
    second_segment_end__ = {X, X, X, 10};
    break;
  default:
    first_segment_begin_ = {X, X, X, X};
    first_segment_end___ = {X, X, X, X};
    second_segment_begin = {X, X, X, X};
    second_segment_end__ = {X, X, X, X};
  }

  const auto c = dr::mp::default_comm().size() - 1;
  auto first_legal_idx = std::max(0, first_segment_begin_[c] - halo_prev);
  auto first_illegal_idx = std::min(12, first_segment_end___[c] + halo_next);
  auto second_legal_idx = std::max(0, second_segment_begin[c] - halo_prev);
  auto second_illegal_idx = std::min(12, second_segment_end__[c] + halo_next);

  if (first_segment_end___[c] == second_segment_begin[c]) {
    // we own the middle segment
    first_illegal_idx = std::min(12, first_segment_end___[c]);
    second_legal_idx = std::max(0, second_segment_begin[c]);
  }

  constexpr size_t N_STEPS = 5;
  auto foreach_fn = [](auto&& elem) { elem *= 10; };
  int expected_multiplier = 1;

  for (size_t i = 0; i < N_STEPS; i++) {
    expected_multiplier *= 10;

    partial_for_each(dv, foreach_fn);
    dv.halo().partial_exchange();

    for (int idx = first_legal_idx; idx < first_illegal_idx; ++idx) {
      check_matching(dv, idx, idx * expected_multiplier);
    }

    partial_for_each(dv, foreach_fn);
    dv.halo().partial_exchange();

    for (int idx = second_legal_idx; idx < second_illegal_idx; ++idx) {
      check_matching(dv, idx, idx * expected_multiplier);
    }
  }

  DRLOG("checks ok");
}

TYPED_TEST(HaloDual, local_is_accessible_in_halo_region_halo_11__partial) {
  local_is_accessible_in_halo_region__partial<TypeParam>(0, 1);
}
