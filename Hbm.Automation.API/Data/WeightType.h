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
    public value class WeightType
    {
    public:
        WeightType(double net, double gross, double tare)
            : _net(net), _gross(gross), _tare(tare)
        {
        }

        void Update(double net, double gross, double tare)
        {
            _net = net;
            _gross = gross;
            _tare = tare;
        }

        property double Net
        {
            double get() { return _net; }
        }

        property double Gross
        {
            double get() { return _gross; }
        }

        property double Tare
        {
            double get() { return _tare; }
        }

    private:
        double _net;
        double _gross;
        double _tare;
    };
}
}
}
}
