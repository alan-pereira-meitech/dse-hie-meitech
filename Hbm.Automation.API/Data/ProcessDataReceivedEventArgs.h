#pragma once

#include "pch.h"
#include "IProcessData.h"

namespace Hbm
{
namespace Automation
{
namespace Api
{
namespace Data
{
    public ref class ProcessDataReceivedEventArgs : EventArgs
    {
    public:
        ProcessDataReceivedEventArgs(IProcessData^ processData)
        {
            ProcessData = processData;
        }

        property IProcessData^ ProcessData;
    };
}
}
}
}
