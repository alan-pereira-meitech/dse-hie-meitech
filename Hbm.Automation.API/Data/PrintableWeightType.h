#pragma once

#include "pch.h"

namespace Hbm
{
namespace Automation
{
namespace Api
{
namespace Data
{
    public value class PrintableWeightType
    {
    public:
        PrintableWeightType(double net, double gross, double tare, int decimals)
            : _net(net), _gross(gross), _tare(tare), _decimals(decimals)
        {
        }

        void Update(double net, double gross, double tare, int decimals)
        {
            _net = net;
            _gross = gross;
            _tare = tare;
            _decimals = decimals;
        }

        property String^ Net
        {
            String^ get() { return ToString(_net); }
        }

        property String^ Gross
        {
            String^ get() { return ToString(_gross); }
        }

        property String^ Tare
        {
            String^ get() { return ToString(_tare); }
        }

    private:
        String^ ToString(double value)
        {
            return value.ToString("F" + _decimals.ToString(), System::Globalization::CultureInfo::InvariantCulture);
        }

        double _net;
        double _gross;
        double _tare;
        int _decimals;
    };
}
}
}
}
