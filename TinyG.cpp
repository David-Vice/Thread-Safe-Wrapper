#include "TinyG.h"
#include <Windows.h>
#include <time.h>
#include "win32comm.h"
#include "optel_tinyg_api.h"
#include <msclr/marshal.h>

using namespace TinyGLib;
using namespace msclr::interop;

TinyG::TinyG()
{
    m_Mutex = gcnew System::Threading::Mutex();
}

TinyG::~TinyG()
{
    delete m_Mutex;
}

double TinyG::Version()
{
    m_Mutex->WaitOne();
    double version = tg_version();
    m_Mutex->ReleaseMutex();
    return version;
}

array<double>^ TinyG::GetPositions()
{
    m_Mutex->WaitOne();

    double positions[MM];
    tg_getpos(positions);
    array<double>^ managedPositions = gcnew array<double>(MM);
    for (int i = 0; i < MM; i++)
    {
        managedPositions[i] = positions[i];
    }

    m_Mutex->ReleaseMutex();
    return managedPositions;
}

bool TinyG::Home(array<bool>^ motors, int timeoutSeconds)
{
    m_Mutex->WaitOne();

    bool home[MM];
    for (int i = 0; i < MM; i++)
    {
        home[i] = motors[i];
    }
    bool result = tg_home(home, timeoutSeconds);

    m_Mutex->ReleaseMutex();
    return result;
}

bool TinyG::Move(array<bool>^ motors, array<double>^ positions, int timeoutSeconds)
{
    m_Mutex->WaitOne();

    bool move[MM];
    double pos[MM];
    for (int i = 0; i < MM; i++)
    {
        move[i] = motors[i];
        pos[i] = positions[i];
    }
    bool result = tg_move(move, pos, timeoutSeconds);

    m_Mutex->ReleaseMutex();
    return result;
}

array<TgRange>^ TinyG::GetRanges()
{
    m_Mutex->WaitOne();

    tg_range_t mrange[MM];
    tg_getranges(mrange);
    array<TgRange>^ managedRanges = gcnew array<TgRange>(MM);
    for (int i = 0; i < MM; i++)
    {
        TgRange managedRange;
        managedRange.Min = mrange[i].min;
        managedRange.Max = mrange[i].max;
        managedRanges[i] = managedRange;
    }

    m_Mutex->ReleaseMutex();
    return managedRanges;
}

void TinyG::Comm(System::String^ message)
{
    m_Mutex->WaitOne();

    marshal_context context;
    const char* nativeMessage = context.marshal_as<const char*>(message);
    tg_comm(const_cast<char*>(nativeMessage));

    m_Mutex->ReleaseMutex();
}

//int main(array<System::String^>^ args)
//{
//    TinyG tgWrapper;
//    tgWrapper.Version();
//    return(0);
//}