// Copyright 2020 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include "rcutils/allocator.h"
#include "rcutils/strdup.h"

#include "rmw/rmw.h"
#include "rmw/error_handling.h"

#include "test_msgs/msg/basic_types.h"

#ifdef RMW_IMPLEMENTATION
# define CLASSNAME_(NAME, SUFFIX) NAME ## __ ## SUFFIX
# define CLASSNAME(NAME, SUFFIX) CLASSNAME_(NAME, SUFFIX)
#else
# define CLASSNAME(NAME, SUFFIX) NAME
#endif

class CLASSNAME (TestSubscription, RMW_IMPLEMENTATION) : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rmw_ret_t ret = rmw_init_options_init(&init_options, rcutils_get_default_allocator());
    ASSERT_EQ(RMW_RET_OK, ret) << rcutils_get_error_string().str;
    init_options.enclave = rcutils_strdup("/", rcutils_get_default_allocator());
    ASSERT_STREQ("/", init_options.enclave);
    ret = rmw_init(&init_options, &context);
    ASSERT_EQ(RMW_RET_OK, ret) << rcutils_get_error_string().str;
    constexpr char node_name[] = "my_test_node";
    constexpr char node_namespace[] = "/my_test_ns";
    node = rmw_create_node(&context, node_name, node_namespace);
    ASSERT_NE(nullptr, node) << rcutils_get_error_string().str;
  }

  void TearDown() override
  {
    rmw_ret_t ret = rmw_destroy_node(node);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_shutdown(&context);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_context_fini(&context);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_init_options_fini(&init_options);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  }

  rmw_init_options_t init_options{rmw_get_zero_initialized_init_options()};
  rmw_context_t context{rmw_get_zero_initialized_context()};
  rmw_node_t * node{nullptr};
};

TEST_F(CLASSNAME(TestSubscription, RMW_IMPLEMENTATION), create_and_destroy) {
  rmw_subscription_options_t options = rmw_get_default_subscription_options();
  constexpr char topic_name[] = "/test";
  const rosidl_message_type_support_t * ts =
    ROSIDL_GET_MSG_TYPE_SUPPORT(test_msgs, msg, BasicTypes);
  rmw_subscription_t * sub =
    rmw_create_subscription(node, ts, topic_name, &rmw_qos_profile_default, &options);
  ASSERT_NE(nullptr, sub) << rmw_get_error_string().str;
  rmw_ret_t ret = rmw_destroy_subscription(node, sub);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
}

TEST_F(CLASSNAME(TestSubscription, RMW_IMPLEMENTATION), create_and_destroy_native) {
  rmw_subscription_options_t options = rmw_get_default_subscription_options();
  constexpr char topic_name[] = "test";
  const rosidl_message_type_support_t * ts =
    ROSIDL_GET_MSG_TYPE_SUPPORT(test_msgs, msg, BasicTypes);
  rmw_qos_profile_t native_qos_profile = rmw_qos_profile_default;
  native_qos_profile.avoid_ros_namespace_conventions = true;
  rmw_subscription_t * sub =
    rmw_create_subscription(node, ts, topic_name, &native_qos_profile, &options);
  ASSERT_NE(nullptr, sub) << rmw_get_error_string().str;
  rmw_ret_t ret = rmw_destroy_subscription(node, sub);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
}

TEST_F(CLASSNAME(TestSubscription, RMW_IMPLEMENTATION), create_with_bad_arguments) {
  rmw_subscription_options_t options = rmw_get_default_subscription_options();
  constexpr char topic_name[] = "/test";
  const rosidl_message_type_support_t * ts =
    ROSIDL_GET_MSG_TYPE_SUPPORT(test_msgs, msg, BasicTypes);
  rmw_subscription_t * sub =
    rmw_create_subscription(nullptr, ts, topic_name, &rmw_qos_profile_default, &options);
  EXPECT_EQ(nullptr, sub);
  rmw_reset_error();

  sub = rmw_create_subscription(node, nullptr, topic_name, &rmw_qos_profile_default, &options);
  EXPECT_EQ(nullptr, sub);
  rmw_reset_error();

  const char * implementation_identifier = node->implementation_identifier;
  node->implementation_identifier = "not-an-rmw-implementation-identifier";
  sub = rmw_create_subscription(node, ts, topic_name, &rmw_qos_profile_default, &options);
  node->implementation_identifier = implementation_identifier;
  EXPECT_EQ(nullptr, sub);
  rmw_reset_error();

  sub = rmw_create_subscription(node, ts, nullptr, &rmw_qos_profile_default, &options);
  EXPECT_EQ(nullptr, sub);
  rmw_reset_error();

  constexpr char topic_name_with_spaces[] = "/foo bar";
  sub =
    rmw_create_subscription(node, ts, topic_name_with_spaces, &rmw_qos_profile_default, &options);
  EXPECT_EQ(nullptr, sub);
  rmw_reset_error();

  constexpr char relative_topic_name[] = "foo";
  sub = rmw_create_subscription(node, ts, relative_topic_name, &rmw_qos_profile_default, &options);
  EXPECT_EQ(nullptr, sub);
  rmw_reset_error();

  sub = rmw_create_subscription(node, ts, topic_name, nullptr, &options);
  EXPECT_EQ(nullptr, sub);
  rmw_reset_error();

  sub = rmw_create_subscription(node, ts, topic_name, &rmw_qos_profile_unknown, &options);
  EXPECT_EQ(nullptr, sub);
  rmw_reset_error();

  sub = rmw_create_subscription(node, ts, topic_name, &rmw_qos_profile_default, nullptr);
  EXPECT_EQ(nullptr, sub);
  rmw_reset_error();

  // Creating and destroying a subscription still succeeds.
  sub = rmw_create_subscription(node, ts, topic_name, &rmw_qos_profile_default, &options);
  ASSERT_NE(nullptr, sub) << rmw_get_error_string().str;
  rmw_ret_t ret = rmw_destroy_subscription(node, sub);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
}

TEST_F(CLASSNAME(TestSubscription, RMW_IMPLEMENTATION), destroy_with_bad_arguments) {
  rmw_subscription_options_t options = rmw_get_default_subscription_options();
  constexpr char topic_name[] = "/test";
  const rosidl_message_type_support_t * ts =
    ROSIDL_GET_MSG_TYPE_SUPPORT(test_msgs, msg, BasicTypes);
  rmw_subscription_t * sub =
    rmw_create_subscription(node, ts, topic_name, &rmw_qos_profile_default, &options);
  ASSERT_NE(nullptr, sub) << rmw_get_error_string().str;

  // Destroying subscription with invalid arguments fails.
  rmw_ret_t ret = rmw_destroy_subscription(nullptr, sub);
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, ret);
  rmw_reset_error();

  ret = rmw_destroy_subscription(node, nullptr);
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, ret);
  rmw_reset_error();

  const char * implementation_identifier = node->implementation_identifier;
  node->implementation_identifier = "not-an-rmw-implementation-identifier";
  ret = rmw_destroy_subscription(node, sub);
  node->implementation_identifier = implementation_identifier;
  EXPECT_EQ(RMW_RET_INCORRECT_RMW_IMPLEMENTATION, ret);
  rmw_reset_error();

  // Destroying subscription still succeeds.
  ret = rmw_destroy_subscription(node, sub);
  EXPECT_EQ(RMW_RET_OK, ret);
  rmw_reset_error();
}

TEST_F(CLASSNAME(TestSubscription, RMW_IMPLEMENTATION), get_actual_qos_from_system_defaults) {
  rmw_subscription_options_t options = rmw_get_default_subscription_options();
  constexpr char topic_name[] = "/test";
  const rosidl_message_type_support_t * ts =
    ROSIDL_GET_MSG_TYPE_SUPPORT(test_msgs, msg, BasicTypes);
  rmw_subscription_t * sub =
    rmw_create_subscription(node, ts, topic_name, &rmw_qos_profile_system_default, &options);
  ASSERT_NE(nullptr, sub) << rmw_get_error_string().str;
  rmw_qos_profile_t qos_profile = rmw_qos_profile_unknown;
  rmw_ret_t ret = rmw_subscription_get_actual_qos(sub, &qos_profile);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  // Check that a valid QoS policy has been put in place for each system default one.
  EXPECT_NE(rmw_qos_profile_system_default.history, qos_profile.history);
  EXPECT_NE(rmw_qos_profile_unknown.history, qos_profile.history);
  EXPECT_NE(rmw_qos_profile_system_default.reliability, qos_profile.reliability);
  EXPECT_NE(rmw_qos_profile_unknown.reliability, qos_profile.reliability);
  EXPECT_NE(rmw_qos_profile_system_default.durability, qos_profile.durability);
  EXPECT_NE(rmw_qos_profile_unknown.durability, qos_profile.durability);
  EXPECT_NE(rmw_qos_profile_system_default.liveliness, qos_profile.liveliness);
  EXPECT_NE(rmw_qos_profile_unknown.liveliness, qos_profile.liveliness);
  EXPECT_NE(rmw_qos_profile_system_default.liveliness, qos_profile.liveliness);
  EXPECT_NE(rmw_qos_profile_unknown.liveliness, qos_profile.liveliness);
  ret = rmw_destroy_subscription(node, sub);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
}

class CLASSNAME (TestSubscriptionUse, RMW_IMPLEMENTATION)
  : public CLASSNAME(TestSubscription, RMW_IMPLEMENTATION)
{
protected:
  using Base = CLASSNAME(TestSubscription, RMW_IMPLEMENTATION);

  void SetUp() override
  {
    Base::SetUp();
    rmw_subscription_options_t options = rmw_get_default_subscription_options();
    sub = rmw_create_subscription(node, ts, topic_name, qos_profile, &options);
    ASSERT_NE(nullptr, sub) << rmw_get_error_string().str;
  }

  void TearDown() override
  {
    rmw_ret_t ret = rmw_destroy_subscription(node, sub);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    Base::TearDown();
  }

  rmw_subscription_t * sub{nullptr};
  const char * const topic_name = "/test";
  const rosidl_message_type_support_t * ts{
    ROSIDL_GET_MSG_TYPE_SUPPORT(test_msgs, msg, BasicTypes)};
  const rmw_qos_profile_t * qos_profile{&rmw_qos_profile_default};
};

TEST_F(CLASSNAME(TestSubscriptionUse, RMW_IMPLEMENTATION), get_actual_qos_with_bad_arguments) {
  rmw_qos_profile_t actual_qos_profile = rmw_qos_profile_unknown;
  rmw_ret_t ret = rmw_subscription_get_actual_qos(nullptr, &actual_qos_profile);
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, ret);
  rmw_reset_error();

  ret = rmw_subscription_get_actual_qos(sub, nullptr);
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, ret);
  rmw_reset_error();

  const char * implementation_identifier = sub->implementation_identifier;
  sub->implementation_identifier = "not-an-rmw-implementation-identifier";
  ret = rmw_subscription_get_actual_qos(sub, &actual_qos_profile);
  EXPECT_EQ(RMW_RET_INCORRECT_RMW_IMPLEMENTATION, ret);
  rmw_reset_error();
  sub->implementation_identifier = implementation_identifier;
}

TEST_F(CLASSNAME(TestSubscriptionUse, RMW_IMPLEMENTATION), get_actual_qos) {
  rmw_qos_profile_t actual_qos_profile = rmw_qos_profile_unknown;
  rmw_ret_t ret = rmw_subscription_get_actual_qos(sub, &actual_qos_profile);
  EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  EXPECT_EQ(rmw_qos_profile_default.history, actual_qos_profile.history);
  EXPECT_EQ(rmw_qos_profile_default.depth, actual_qos_profile.depth);
  EXPECT_EQ(rmw_qos_profile_default.reliability, actual_qos_profile.reliability);
  EXPECT_EQ(rmw_qos_profile_default.durability, actual_qos_profile.durability);
}
