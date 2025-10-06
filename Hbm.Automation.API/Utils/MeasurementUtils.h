#pragma once

namespace Hbm
{
namespace Automation
{
namespace Api
{
namespace Utils
{
    public ref class MeasurementUtils abstract sealed
    {
    public:
        static double DigitToDouble(int value, int decimals)
        {
            double scale = System::Math::Pow(10.0, -decimals);
            return value * scale;
        }

        static int DoubleToDigit(double value, int decimals)
        {
            double scale = System::Math::Pow(10.0, decimals);
            return static_cast<int>(System::Math::Round(value * scale));
        }
    };
}
}
}
}
