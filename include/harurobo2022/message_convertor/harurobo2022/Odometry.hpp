#pragma once

#include "harurobo2022/Odometry.h"

#include "../../lib/reverse_buffer.hpp"
#include "../template.hpp"


namespace Harurobo2022
{
    namespace
    {
        template<>
        struct MessageConvertor<harurobo2022::Odometry>
        {
            using Message = harurobo2022::Odometry;

            struct RawData final
            {
                float pos_x{};
                float pos_y{};
                float rot_z{};
            };

            struct CanData final
            {
                StewLib::ReverseBuffer<float> pos_x{};
                StewLib::ReverseBuffer<float> pos_y{};
                StewLib::ReverseBuffer<float> rot_z{};
                
                CanData() = default;

                void reverse() noexcept
                {
                    pos_x.reverse();
                    pos_y.reverse();
                    rot_z.reverse();
                }
            };

            RawData raw_data;

            MessageConvertor() = default;
            MessageConvertor(const MessageConvertor&) = default;
            MessageConvertor(MessageConvertor&&) = default;
            MessageConvertor& operator=(const MessageConvertor&) = default;
            MessageConvertor& operator=(MessageConvertor&&) = default;
            ~MessageConvertor() = default;

            constexpr MessageConvertor(const Message& msg) noexcept:
                MessageConvertor({msg.pos_x, msg.pos_y, msg.rot_z})
            {}

            constexpr MessageConvertor(const RawData& raw_data) noexcept:
                raw_data{raw_data}
            {}

            MessageConvertor(CanData can_data) noexcept
            {
                can_data.reverse();
            }

            operator Message() const noexcept
            {
                Message msg;
                msg.pos_x = raw_data.pos_x;
                msg.pos_y = raw_data.pos_y;
                msg.rot_z = raw_data.rot_z;

                return msg;
            }
        };
    }
}