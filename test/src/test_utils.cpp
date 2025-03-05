/**
 *  Copyright (C) 2021 Huawei Technologies Co, Ltd.
 */

#include "test_utils.h"

#include <iomanip>
#include <sstream>

#include <base/containers/string.h>

bool operator>(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs)
{
    return ((lhs.x > rhs.x) && (lhs.y > rhs.y));
}

bool operator<(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs)
{
    return ((lhs.x < rhs.x) && (lhs.y < rhs.y));
}

bool operator>(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs)
{
    return ((lhs.x > rhs.x) && (lhs.y > rhs.y));
}

bool operator<(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs)
{
    return ((lhs.x < rhs.x) && (lhs.y < rhs.y));
}

bool operator>=(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs)
{
    return ((lhs.x >= rhs.x) && (lhs.y >= rhs.y));
}

bool operator<=(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs)
{
    return ((lhs.x <= rhs.x) && (lhs.y <= rhs.y));
}

bool operator>=(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs)
{
    return ((lhs.x >= rhs.x) && (lhs.y >= rhs.y));
}

bool operator<=(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs)
{
    return ((lhs.x <= rhs.x) && (lhs.y <= rhs.y));
}

bool AreEqual(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs, float epsilon)
{
    return BASE_NS::Math::abs(lhs.x - rhs.x) < epsilon && BASE_NS::Math::abs(lhs.y - rhs.y) < epsilon &&
           BASE_NS::Math::abs(lhs.z - rhs.z) < epsilon;
}

bool AreNear(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs)
{
    return AreEqual(lhs, rhs, 0.01f);
}

bool AreEqual(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs, float epsilon)
{
    return BASE_NS::Math::abs(lhs.x - rhs.x) < epsilon && BASE_NS::Math::abs(lhs.y - rhs.y) < epsilon;
}

bool AreNear(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs)
{
    return AreEqual(lhs, rhs, 0.01f);
}

namespace BASE_NS {

std::ostream& operator<<(std::ostream& os, const BASE_NS::Uid& uid)
{
    std::stringstream ss;

    ss << "{" << std::setfill('0') << std::setw(2) << std::hex;
    for (size_t i = 0; i < sizeof(uid.data); i++) {
        ss << static_cast<uint16_t>(uid.data[i]);
        if (i == 3 || i == 5 || i == 7 || i == 9) {
            ss << "-";
        }
    }
    ss << "}";
    return os << ss.str();
}

std::ostream& operator<<(std::ostream& os, const BASE_NS::string& s)
{
    return os << s.c_str();
}

std::ostream& operator<<(std::ostream& os, const BASE_NS::string_view& s)
{
    return os << s.data();
}

namespace Math {

std::ostream& operator<<(std::ostream& os, const BASE_NS::Math::Vec3& vec)
{
    std::stringstream ss;
    ss << std::setprecision(6) << "BASE_NS::Math::Vec3 {" << vec.x << ", " << vec.y << ", " << vec.z << "}";
    return os << ss.str();
}

std::ostream& operator<<(std::ostream& os, const BASE_NS::Math::Vec2& vec)
{
    std::stringstream ss;
    ss << std::setprecision(6) << "BASE_NS::Math::Vec2 {" << vec.x << ", " << vec.y << "}";
    return os << ss.str();
}

} // namespace Math
} // namespace BASE_NS

namespace META_NS {
std::ostream& operator<<(std::ostream& os, const META_NS::TimeSpan& ts)
{
    return os << ts.ToMicroseconds() << "us";
}

} // namespace META_NS
