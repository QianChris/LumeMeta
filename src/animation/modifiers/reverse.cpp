#include "reverse.h"

META_BEGIN_NAMESPACE()

namespace AnimationModifiers {

bool Reverse::Build(const IMetadata::Ptr& data)
{
    return true;
}

bool Reverse::ProcessOnGetDuration(DurationData& duration) const
{
    return true;
}
bool Reverse::ProcessOnStep(StepData& step) const
{
    step.progress = 1.f - step.progress;
    step.reverse = !step.reverse;
    return true;
}

} // namespace AnimationModifiers

META_END_NAMESPACE()
