#pragma once
#include <msclr/marshal.h>

namespace TinyGLib {

    public value struct TgRange
    {
        double Min;
        double Max;
    };

    public ref class TinyG
    {
    public:
        TinyG(); // Constructor
        ~TinyG(); // Destructor
        double Version();
        array<double>^ GetPositions();
        bool Home(array<bool>^ motors, int timeoutSeconds);
        bool Move(array<bool>^ motors, array<double>^ positions, int timeoutSeconds);
        array<TgRange>^ GetRanges();
        void Comm(System::String^ message);
        bool OpenPorts();
        void ClosePorts();

    private:
        System::Threading::Mutex^ m_Mutex; // Mutex member
    };
}