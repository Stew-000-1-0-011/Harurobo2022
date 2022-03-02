/*

*/

#include <std_msgs/UInt8.h>

#include "harurobo2022/timer.hpp"
#include "harurobo2022/state.hpp"
#include "harurobo2022/active_manager.hpp"
#include "harurobo2022/can_publisher.hpp"
#include "harurobo2022/motors.hpp"
#include "harurobo2022/topics.hpp"
#include "harurobo2022/chart.hpp"

using namespace StewLib;
using namespace Harurobo2022;

namespace
{
    class AutoCommanderNode final
    {
        LiftMotors lift_motors{};

        using stepping_motor = CanTxTopic<StringlikeTypes::stepping_motor, std_msgs::UInt8, Config::CanId::Tx::stepping_motor>;
        CanPublisher<stepping_motor> stepping_motor_pub{1};
        struct ShovelCmd final
        {
            enum Enum : std::uint8_t
            {
                open = 0,
                close
            };
        };

        using table_cloth = CanTxTopic<StringlikeTypes::table_cloth, std_msgs::UInt8, Config::CanId::Tx::table_cloth>;
        CanPublisher<table_cloth> table_cloth_pub{1};
        struct TableClothCmd
        {
            enum Enum : std::uint8_t
            {
                push = 0,
                pull
            };
        };

        Subscriber<Topics::odometry> odometry_sub{1, [this](const typename Topics::odometry::Message::ConstPtr& msg_p) noexcept { odometry_callback(msg_p); }};
        
        Timer check_position{1.0 / Config::ExecutionInterval::auto_commander_freq, [this](const auto& event){ check_position_callback(event); }};

        StateManager state_manager{};

        ActiveManager
        <
            StringlikeTypes::auto_commander,
            decltype(lift_motors), decltype(odometry_sub), decltype(check_position)
        > active_manager
        {
            lift_motors, odometry_sub, check_position
        };

        // ひとまずは位置と姿勢を速度上限つきP制御で追う。目標位置姿勢と現在位置姿勢の差を定数倍して並進速度角速度にする。
        Vec2D<float> now_pos{};
        float now_rot_z{};
        
        std::list<Command>::const_iterator target_iter{trajectory.cbegin()};
        std::list<std::list<Command>>::iterator missions_iter{steps.begin()};

    public:
        AutoCommanderNode() noexcept
        {
            active_manager.deactivate();
        }

    private:
        void odometry_callback(const Topics::odometry::Message::ConstPtr& msg_p) noexcept
        {
            now_pos = {msg_p->pos_x, msg_p->pos_y};
            now_rot_z = msg_p->rot_z;
        }

        void check_position_callback(const ros::TimerEvent&) noexcept
        {
            if(missions_iter->empty()) return;

            for(auto mission_iter = missions_iter->begin(); mission_iter != missions_iter->end();)
            {
                if(mission_iter->pass_near_circle.is_in(now_pos))
                {
                    do_work(mission_iter->work);
                    mission_iter = missions_iter->erase(mission_iter);
                }
                else
                {
                    ++mission_iter;
                }
            }
        }

        /* TODO over_fenceの実装 */
        void do_work(const Work work) noexcept
        {
            switch(work)
            {
            case Work::transit:
                break;
            
            case Work::collector_bottom:
                case_collector_bottom();
                break;
            
            case Work::collector_step1:
                case_collector_step1();
                break;

            case Work::collector_step2:
                case_collector_step2();
                break;
            
            case Work::collector_step3:
                case_collector_step3();
                break;

            case Work::collector_shovel_open:
                case_collector_shovel_open();
                break;

            case Work::collector_shovel_close:
                case_collector_shovel_close();
                break;
            
            case Work::collector_tablecloth_push:
                case_collector_tablecloth_push();
                break;

            case Work::collector_tablecloth_pull:
                case_collector_tablecloth_pull();
                break;

            case Work::change_to_over_fence:
                break;
            }
        }

        void case_collector_bottom() noexcept
        {
            lift_motors.collector_pub.send_target(Config::collector_bottom_position);
        }

        void case_collector_step1() noexcept
        {
            lift_motors.collector_pub.send_target(Config::collector_step1_position);
        }

        void case_collector_step2() noexcept
        {
            lift_motors.collector_pub.send_target(Config::collector_step2_position);
        }

        void case_collector_step3() noexcept
        {
            lift_motors.collector_pub.send_target(Config::collector_step3_position);
        }

        void case_collector_shovel_open() noexcept
        {
            stepping_motor_pub.can_publish(ShovelCmd::open);
        }

        void case_collector_shovel_close() noexcept
        {
            stepping_motor_pub.can_publish(ShovelCmd::close);
        }

        void case_collector_tablecloth_push() noexcept
        {
            table_cloth_pub.can_publish(TableClothCmd::push);
        }

        void case_collector_tablecloth_pull() noexcept
        {
            table_cloth_pub.can_publish(TableClothCmd::pull);
        }
    };
}

int main(int argc, char ** argv)
{
    ros::init(argc, argv, StringlikeTypes::auto_commander::str);
    StaticInitDeinit satic_init_deinit;

    AutoCommanderNode auto_commander_node;

    ROS_INFO("%s node has started.", StringlikeTypes::auto_commander::str);

    ros::spin();

    ROS_INFO("%s node has terminated.", StringlikeTypes::auto_commander::str);
}