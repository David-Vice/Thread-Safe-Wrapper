#include "TinyG.h"
#include <Windows.h>
#include <time.h>
#include "win32comm.h"
#include "optel_tinyg_api.h"

using namespace TinyGLib;
using namespace msclr::interop;

public ref class scoped_lock {
public:
    scoped_lock(System::Threading::Mutex^ m) : m_Mutex(m) {
        m_Mutex->WaitOne();
        printf("Closed Mutex\n");
    }

    ~scoped_lock() {
        m_Mutex->ReleaseMutex();
        printf("Released Mutex\n");
    }

private:
    System::Threading::Mutex^ m_Mutex;
};

TinyG::TinyG()
{
    m_Mutex = gcnew System::Threading::Mutex();
    printf("TinyG Constructor\n");
}

TinyG::~TinyG()
{
    delete m_Mutex;
    printf("TinyG Destructor\n");
}

double TinyG::Version()
{
    scoped_lock lock(m_Mutex);
    printf("Version()\n");
    double version = tg_version();
    return version;
}

array<double>^ TinyG::GetPositions()
{
    scoped_lock lock(m_Mutex);
    printf("GetPositions()\n");
    double positions[MM];
    tg_getpos(positions);
    array<double>^ managedPositions = gcnew array<double>(MM);
    for (int i = 0; i < MM; i++)
    {
        managedPositions[i] = positions[i];
    }

    return managedPositions;
}

bool TinyG::Home(array<bool>^ motors, int timeoutSeconds)
{
    scoped_lock lock(m_Mutex);
    printf("Home()\n");
    bool home[MM];
    for (int i = 0; i < MM; i++)
    {
        home[i] = motors[i];
    }
    bool result = tg_home(home, timeoutSeconds);

    return result;
}

bool TinyG::Move(array<bool>^ motors, array<double>^ positions, int timeoutSeconds)
{
    scoped_lock lock(m_Mutex);
    printf("Move()\n");
    bool move[MM];
    double pos[MM];
    for (int i = 0; i < MM; i++)
    {
        move[i] = motors[i];
        pos[i] = positions[i];
    }
    bool result = tg_move(move, pos, timeoutSeconds);

    return result;
}

array<TgRange>^ TinyG::GetRanges()
{
    scoped_lock lock(m_Mutex);
    printf("GetRanges()\n");
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

    return managedRanges;
}

void TinyG::Comm(System::String^ message)
{
    scoped_lock lock(m_Mutex);
    printf("Comm()\n");
    marshal_context context;
    const char* nativeMessage = context.marshal_as<const char*>(message);
    tg_comm(const_cast<char*>(nativeMessage));
}

bool TinyG::OpenPorts()
{
    scoped_lock lock(m_Mutex);
    printf("OpenPorts()\n");
    return tg_open_ports();
}

void TinyG::ClosePorts()
{
    scoped_lock lock(m_Mutex);
    printf("ClosePorts()\n");
    tg_close_ports();
}