
#include <ros/ros.h>

#include "harurobo2022/lib/shirasu_util.hpp"

#include "harurobo2022/topics.hpp"
#include "harurobo2022/state.hpp"
#include "harurobo2022/activate_publisher.hpp"
#include "harurobo2022/can_publisher.hpp"

using namespace Harurobo2022;

const char *const node_name = "state_manager";

class StateManagerNode final
{
    ros::NodeHandle nh{};

    ros::Publisher can_tx_pub{nh.advertise<Topics::can_tx::Message>(Topics::can_tx::topic, 20)};

    ros::Publisher state_pub{nh.advertise<Topics::state_::Message>(Topics::state_::topic, 1)};
    ros::Publisher shutdown_pub{nh.advertise<Topics::shutdown_::Message>(Topics::shutdown_::topic, 1)};

    ros::Subscriber state_sub{nh.subscribe<Topics::state_::Message>(Topics::state_::topic, 10, &StateManagerNode::state_callback, this)};

    ActivatePublisher under_carriage_4wheel_activatepub{"under_carriage_4wheel", nh};
    ActivatePublisher auto_commander_activatepub{"auto_commander", nh};

    State state{State::desable};

public:
    StateManagerNode() = default;

    inline void startup() noexcept;
    inline void motors_activate() noexcept;
    inline void motors_deactivate() noexcept;
    inline void motors_reset() noexcept;

private:
    inline void state_callback(const Topics::state_::Message::ConstPtr& msg_p) noexcept;
    
    inline void case_shutdown() noexcept;
    inline void case_manual() noexcept;
    inline void case_reset() noexcept;
    inline void case_automatic() noexcept;
    inline void case_desable() noexcept;
};

inline void StateManagerNode::state_callback(const Topics::state_::Message::ConstPtr& msg_p) noexcept
{
    ROS_INFO("state is subscribed.");
    switch(static_cast<State>(msg_p->data))
    {
    case State::desable:
        case_desable();
        break;

    case State::shutdown:
        case_shutdown();
        break;
    
    case State::manual:
        case_manual();
        break;

    case State::reset:
        case_reset();
        break;

    case State::automatic:
        case_automatic();
        break;
    
    default:
        break;
    }
}

inline void StateManagerNode::case_shutdown() noexcept
{
    ROS_INFO("State changed to ShutDown.");

    shutdown_pub.publish(Topics::shutdown_::Message());
    ros::shutdown();
}

inline void StateManagerNode::case_manual() noexcept
{
    ROS_INFO("State changed to Manual.");

    under_carriage_4wheel_activatepub.activate();
    auto_commander_activatepub.deactivate();

    motors_activate();
}

inline void StateManagerNode::case_reset() noexcept
{
    ROS_INFO("State changed to Reset.");

    under_carriage_4wheel_activatepub.deactivate();
    auto_commander_activatepub.deactivate();

    motors_reset();
}

inline void StateManagerNode::case_automatic() noexcept
{
    ROS_INFO("State changed to Automatic.");

    under_carriage_4wheel_activatepub.activate();
    auto_commander_activatepub.activate();

    motors_activate();
}

inline void StateManagerNode::case_desable() noexcept
{
    ROS_INFO("State changed to Desable.");
    under_carriage_4wheel_activatepub.deactivate();
    auto_commander_activatepub.deactivate();

    motors_deactivate();
}

inline void StateManagerNode::motors_activate() noexcept
{
    for(std::size_t i = 0; i < Config::CanId::Tx::position_controll_ids_size; ++i)
    {
        can_publish<std_msgs::UInt8>(can_tx_pub, Config::CanId::Tx::position_controll_ids[i], ShirasuUtil::position_mode);
    }

    for(int i = 0; i < 4; ++i)
    {
        can_publish<std_msgs::UInt8>(can_tx_pub, Config::CanId::Tx::DriveMotor::all[i], ShirasuUtil::velocity_mode);
    }
}

inline void StateManagerNode::motors_deactivate() noexcept
{
    for(std::size_t i = 0; i < Config::CanId::Tx::position_controll_ids_size; ++i)
    {
        can_publish<std_msgs::UInt8>(can_tx_pub, Config::CanId::Tx::position_controll_ids[i], ShirasuUtil::disable_mode);
    }

    for(int i = 0; i < 4; ++i)
    {
        can_publish<std_msgs::UInt8>(can_tx_pub, Config::CanId::Tx::DriveMotor::all[i], ShirasuUtil::disable_mode);
    }
}

inline void StateManagerNode::motors_reset() noexcept
{
    for(std::size_t i = 0; i < Config::CanId::Tx::position_controll_ids_size; ++i)
    {
        can_publish<std_msgs::UInt8>(can_tx_pub, Config::CanId::Tx::position_controll_ids[i], ShirasuUtil::homing_mode);
    }

    for(int i = 0; i < 4; ++i)
    {
        can_publish<std_msgs::UInt8>(can_tx_pub, Config::CanId::Tx::DriveMotor::all[i], ShirasuUtil::disable_mode);
    }
}

inline void StateManagerNode::startup() noexcept
{
    case_reset();
}

int main(int argc, char ** argv)
{
    ros::init(argc, argv, node_name);

    StateManagerNode state_manager_node;

    ROS_INFO("%s node has started.", node_name);

    ros::spin();

    ROS_INFO("%s node has terminated.", node_name);
}