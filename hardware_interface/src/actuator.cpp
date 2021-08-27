// Copyright 2020 ros2_control Development Team
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

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "hardware_interface/actuator.hpp"
#include "hardware_interface/actuator_interface.hpp"

#include "hardware_interface/handle.hpp"
#include "hardware_interface/hardware_info.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "hardware_interface/types/hardware_interface_status_values.hpp"
#include "hardware_interface/types/lifecycle_state_names.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp_lifecycle/node_interfaces/lifecycle_node_interface.hpp"
#include "rclcpp_lifecycle/state.hpp"

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

namespace hardware_interface
{
Actuator::Actuator(std::unique_ptr<ActuatorInterface> impl)
: impl_(std::move(impl)),
  lifecycle_state_(rclcpp_lifecycle::State(
    lifecycle_msgs::msg::State::PRIMARY_STATE_UNKNOWN, lifecycle_state_names::UNKNOWN))
{
}

rclcpp_lifecycle::State Actuator::initialize(const HardwareInfo & actuator_info)
{
  if (lifecycle_state_.id() == lifecycle_msgs::msg::State::PRIMARY_STATE_UNKNOWN)
  {
    switch (impl_->on_init(actuator_info))
    {
      case CallbackReturn::SUCCESS:
        lifecycle_state_ = rclcpp_lifecycle::State(
          lifecycle_msgs::msg::State::PRIMARY_STATE_UNCONFIGURED,
          lifecycle_state_names::UNCONFIGURED);
        break;
      case CallbackReturn::FAILURE:
      case CallbackReturn::ERROR:
        lifecycle_state_ = rclcpp_lifecycle::State(
          lifecycle_msgs::msg::State::PRIMARY_STATE_FINALIZED, lifecycle_state_names::FINALIZED);
        break;
    }
  }
  return lifecycle_state_;
}

rclcpp_lifecycle::State Actuator::configure()
{
  if (lifecycle_state_.id() == lifecycle_msgs::msg::State::PRIMARY_STATE_UNCONFIGURED)
  {
    switch (impl_->on_configure())
    {
      case CallbackReturn::SUCCESS:
        lifecycle_state_ = rclcpp_lifecycle::State(
          lifecycle_msgs::msg::State::PRIMARY_STATE_INACTIVE, lifecycle_state_names::INACTIVE);
        break;
      case CallbackReturn::FAILURE:
        lifecycle_state_ = rclcpp_lifecycle::State(
          lifecycle_msgs::msg::State::PRIMARY_STATE_UNCONFIGURED,
          lifecycle_state_names::UNCONFIGURED);
        break;
      case CallbackReturn::ERROR:
        lifecycle_state_ = error();
        break;
    }
  }
  return lifecycle_state_;
}

rclcpp_lifecycle::State Actuator::cleanup()
{
  if (lifecycle_state_.id() == lifecycle_msgs::msg::State::PRIMARY_STATE_INACTIVE)
  {
    switch (impl_->on_cleanup())
    {
      case CallbackReturn::SUCCESS:
        lifecycle_state_ = rclcpp_lifecycle::State(
          lifecycle_msgs::msg::State::PRIMARY_STATE_UNCONFIGURED,
          lifecycle_state_names::UNCONFIGURED);
        break;
      case CallbackReturn::FAILURE:
      case CallbackReturn::ERROR:
        lifecycle_state_ = error();
        break;
    }
  }
  return lifecycle_state_;
}

rclcpp_lifecycle::State Actuator::shutdown()
{
  if (
    lifecycle_state_.id() != lifecycle_msgs::msg::State::PRIMARY_STATE_UNKNOWN &&
    lifecycle_state_.id() != lifecycle_msgs::msg::State::PRIMARY_STATE_FINALIZED)
  {
    switch (impl_->on_shutdown(lifecycle_state_))
    {
      case CallbackReturn::SUCCESS:
        lifecycle_state_ = rclcpp_lifecycle::State(
          lifecycle_msgs::msg::State::PRIMARY_STATE_FINALIZED, lifecycle_state_names::FINALIZED);
        break;
      case CallbackReturn::FAILURE:
      case CallbackReturn::ERROR:
        lifecycle_state_ = error();
        break;
    }
  }
  return lifecycle_state_;
}

rclcpp_lifecycle::State Actuator::activate()
{
  if (lifecycle_state_.id() == lifecycle_msgs::msg::State::PRIMARY_STATE_INACTIVE)
  {
    switch (impl_->on_activate())
    {
      case CallbackReturn::SUCCESS:
        lifecycle_state_ = rclcpp_lifecycle::State(
          lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE, lifecycle_state_names::ACTIVE);
        break;
      case CallbackReturn::FAILURE:
        lifecycle_state_ = rclcpp_lifecycle::State(
          lifecycle_msgs::msg::State::PRIMARY_STATE_INACTIVE, lifecycle_state_names::INACTIVE);
        break;
      case CallbackReturn::ERROR:
        lifecycle_state_ = error();
        break;
    }
  }
  return lifecycle_state_;
}

rclcpp_lifecycle::State Actuator::deactivate()
{
  if (lifecycle_state_.id() == lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE)
  {
    switch (impl_->on_deactivate())
    {
      case CallbackReturn::SUCCESS:
        lifecycle_state_ = rclcpp_lifecycle::State(
          lifecycle_msgs::msg::State::PRIMARY_STATE_INACTIVE, lifecycle_state_names::INACTIVE);
        break;
      case CallbackReturn::FAILURE:
        lifecycle_state_ = rclcpp_lifecycle::State(
          lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE, lifecycle_state_names::ACTIVE);
        break;
      case CallbackReturn::ERROR:
        lifecycle_state_ = error();
        break;
    }
  }
  return lifecycle_state_;
}

rclcpp_lifecycle::State Actuator::error()
{
  if (lifecycle_state_.id() != lifecycle_msgs::msg::State::PRIMARY_STATE_UNKNOWN)
  {
    switch (impl_->on_error(lifecycle_state_))
    {
      case CallbackReturn::SUCCESS:
        lifecycle_state_ = rclcpp_lifecycle::State(
          lifecycle_msgs::msg::State::PRIMARY_STATE_UNCONFIGURED,
          lifecycle_state_names::UNCONFIGURED);
        break;
      case CallbackReturn::FAILURE:
      case CallbackReturn::ERROR:
        lifecycle_state_ = rclcpp_lifecycle::State(
          lifecycle_msgs::msg::State::PRIMARY_STATE_FINALIZED, lifecycle_state_names::FINALIZED);
        break;
    }
  }
  return lifecycle_state_;
}

std::vector<StateInterface> Actuator::export_state_interfaces()
{
  // TODO(karsten1987): Might be worth to do some brief sanity check here
  return impl_->export_state_interfaces();
}

std::vector<CommandInterface> Actuator::export_command_interfaces()
{
  // TODO(karsten1987): Might be worth to do some brief sanity check here
  return impl_->export_command_interfaces();
}

return_type Actuator::prepare_command_mode_switch(
  const std::vector<std::string> & start_interfaces,
  const std::vector<std::string> & stop_interfaces)
{
  return impl_->prepare_command_mode_switch(start_interfaces, stop_interfaces);
}

return_type Actuator::perform_command_mode_switch(
  const std::vector<std::string> & start_interfaces,
  const std::vector<std::string> & stop_interfaces)
{
  return impl_->perform_command_mode_switch(start_interfaces, stop_interfaces);
}

std::string Actuator::get_name() const { return impl_->get_name(); }

const rclcpp_lifecycle::State & Actuator::get_state() const { return lifecycle_state_; }

return_type Actuator::read() { return impl_->read(); }

return_type Actuator::write() { return impl_->write(); }

}  // namespace hardware_interface
