/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2013, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/* Author: Ioan Sucan */

#include <moveit/robot_model_loader/robot_model_loader.h>
#include <moveit/robot_state/robot_state.h>
#include <moveit/profiler/profiler.h>
#include <rclcpp/rclcpp.hpp>

using namespace std::chrono_literals;

static const rclcpp::Logger LOGGER = rclcpp::get_logger("evaluate_state_operations_speed");
static const std::string ROBOT_DESCRIPTION = "robot_description";

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("evaluate_state_operations_speed");

  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(node);

  robot_model_loader::RobotModelLoader rml(node, ROBOT_DESCRIPTION);
  rclcpp::sleep_for(500ms);

  moveit::core::RobotModelConstPtr robot_model = rml.getModel();
  if (robot_model)
  {
    static const int N = 10000;
    moveit::core::RobotState state(robot_model);

    printf("Evaluating model '%s' using %d trials for each test\n", robot_model->getName().c_str(), N);

    moveit::tools::Profiler::Clear();
    moveit::tools::Profiler::Start();

    printf("Evaluating FK Default ...\n");
    for (int i = 0; i < N; ++i)
    {
      moveit::tools::Profiler::Begin("FK Default");
      state.setToDefaultValues();
      state.update();
      moveit::tools::Profiler::End("FK Default");
    }

    printf("Evaluating FK Random ...\n");
    for (int i = 0; i < N; ++i)
    {
      moveit::tools::Profiler::Begin("FK Random");
      state.setToRandomPositions();
      state.update();
      moveit::tools::Profiler::End("FK Random");
    }

    std::vector<moveit::core::RobotState*> copies(N, (moveit::core::RobotState*)nullptr);
    printf("Evaluating Copy State ...\n");
    for (int i = 0; i < N; ++i)
    {
      moveit::tools::Profiler::Begin("Copy State");
      copies[i] = new moveit::core::RobotState(state);
      moveit::tools::Profiler::End("Copy State");
    }

    printf("Evaluating Free State ...\n");
    for (int i = 0; i < N; ++i)
    {
      moveit::tools::Profiler::Begin("Free State");
      delete copies[i];
      moveit::tools::Profiler::End("Free State");
    }

    const std::vector<std::string>& groups = robot_model->getJointModelGroupNames();
    for (const std::string& group : groups)
    {
      printf("\n");
      const moveit::core::JointModelGroup* jmg = robot_model->getJointModelGroup(group);

      printf("%s: Evaluating FK Random ...\n", group.c_str());
      std::string pname = group + ":FK Random";
      for (int i = 0; i < N; ++i)
      {
        moveit::tools::Profiler::Begin(pname);
        state.setToRandomPositions(jmg);
        state.update();
        moveit::tools::Profiler::End(pname);
      }
    }

    moveit::tools::Profiler::Stop();
    moveit::tools::Profiler::Status();
  }
  else
    RCLCPP_ERROR(LOGGER, "Unable to initialize robot model.");

  executor.spin();
  rclcpp::shutdown();
  return 0;
}
